# PAGE Iberica PI-T100-M reverse engineering
#
# Nothing here touches the files in original/ - they are the ground truth.

PYTHON  ?= python3
ASL     ?= asl
P2BIN   ?= p2bin

ROM      = original/firmware.bin
BUILD    = build
ASM_SRC  = src/firmware.asm
# Last used ROM address; everything above is erased 0xFF (see tools/rom_info.py).
USED_END = 3047

.DEFAULT_GOAL := help

.PHONY: help
help:  ## Show this help
	@grep -hE '^[a-z0-9_-]+:.*?##' $(MAKEFILE_LIST) \
	  | sed -E 's/:.*## /\t/' \
	  | awk -F'\t' '{printf "  \033[1m%-14s\033[0m %s\n", $$1, $$2}'

.PHONY: check
check: rom-check symbols-check verify  ## Run every non-hardware check

.PHONY: rom-check
rom-check:  ## Verify the ROM dump against the documented checksum
	cd original && sha256sum -c SHA256SUMS
	$(PYTHON) tools/rom_info.py

.PHONY: symbols-check
symbols-check:  ## Validate ghidra/symbols.csv
	@$(PYTHON) -c "import csv, sys; \
	rows = [l for l in open('ghidra/symbols.csv') if not l.startswith('#')]; \
	recs = list(csv.DictReader(rows)); \
	cols = {'space','addr','name','kind','comment'}; \
	bad  = [r for r in recs if set(r) != cols]; \
	sys.exit('bad columns: %r' % bad[:1]) if bad else None; \
	bad  = [r['space'] for r in recs if r['space'] not in ('mem','io')]; \
	sys.exit('bad space: %r' % bad) if bad else None; \
	bad  = [r['kind'] for r in recs if r['kind'] not in ('func','label','data','port')]; \
	sys.exit('bad kind: %r' % bad) if bad else None; \
	[int(r['addr'], 16) for r in recs]; \
	names = [r['name'] for r in recs]; \
	dupes = {n for n in names if names.count(n) > 1}; \
	sys.exit('duplicate names: %r' % dupes) if dupes else None; \
	print('symbols.csv OK - %d symbols (%d mem, %d io)' % (len(recs), \
	      sum(r['space'] == 'mem' for r in recs), \
	      sum(r['space'] == 'io'  for r in recs)))"

.PHONY: font
font:  ## Render the 5x7 glyph table
	$(PYTHON) tools/extract_font.py

.PHONY: font-map
font-map:  ## Regenerate docs/font-map.png (flip-dot glyph map for the README)
	$(PYTHON) tools/extract_font.py --png docs/font-map.png

$(BUILD):
	mkdir -p $(BUILD)

.PHONY: verify
verify: $(BUILD)  ## Reassemble src/ and prove it matches the original ROM
	@set -e; \
	if [ ! -f $(ASM_SRC) ]; then \
	  echo "$(ASM_SRC) does not exist yet - nothing to verify."; \
	  echo "See src/README.md for how to start the matching disassembly."; \
	  exit 0; \
	fi; \
	command -v $(ASL) >/dev/null || { \
	  echo "$(ASL) not found. Install Macro Assembler AS (see src/README.md)."; \
	  exit 1; }; \
	$(ASL) -cpu 8085 -L -o $(BUILD)/firmware.p $(ASM_SRC); \
	$(P2BIN) $(BUILD)/firmware.p $(BUILD)/firmware.bin -r '$$0000-$$$(USED_END)'; \
	head -c $$((0x$(USED_END) + 1)) $(ROM) > $(BUILD)/original_used.bin; \
	if cmp -s $(BUILD)/firmware.bin $(BUILD)/original_used.bin; then \
	  echo "MATCH - reassembly is byte-identical to $(ROM) over 0000-$(USED_END)"; \
	  echo "        (the erased 0xFF tail above that is checked by rom-check)"; \
	else \
	  echo "MISMATCH:"; \
	  cmp -l $(BUILD)/firmware.bin $(BUILD)/original_used.bin | head -20; \
	  exit 1; \
	fi

.PHONY: bringup-selftest
bringup-selftest:  ## Exercise tools/bringup.py against the emulator (~25 s)
	PYTHONPATH=tools $(PYTHON) tools/bringup.py --sim --stage demo --address 5

.PHONY: disasm
disasm:  ## Regenerate src/firmware.asm from the ROM
	$(PYTHON) tools/disasm.py --asm > $(ASM_SRC)

.PHONY: clean
clean:  ## Remove build artefacts
	rm -rf $(BUILD)

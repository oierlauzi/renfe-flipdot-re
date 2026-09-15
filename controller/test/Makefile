# Host-side checks for the parts that are pure arithmetic: the encoder and the
# stopwatch. No ESP32, no toolchain, no panel.
#
#   make          build and run everything
#   make clean
#
# cp437.c and stopwatch.c depend on nothing but the C library precisely so they
# can be compiled here. cp437.c is checked against tools/pit100.py, the code that
# has actually driven the hardware; stopwatch.c against exact minute boundaries,
# which cannot be tested on the board without waiting for them.

CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Werror -O1 -std=c11
PYTHON  ?= python3
ROOT    := $(abspath ../..)
BUILD   := build

.PHONY: all check clean
all: check

$(BUILD)/test_cp437: test_cp437.c ../main/cp437.c | $(BUILD)
	$(CC) $(CFLAGS) -I../main -o $@ $^

$(BUILD)/test_stopwatch: test_stopwatch.c ../main/stopwatch.c | $(BUILD)
	$(CC) $(CFLAGS) -I../main -o $@ $^

$(BUILD):
	@mkdir -p $(BUILD)

check: $(BUILD)/test_cp437 $(BUILD)/test_stopwatch
	@$(BUILD)/test_stopwatch
	@$(BUILD)/test_cp437 layout
	@$(BUILD)/test_cp437 sevenbit
	@$(BUILD)/test_cp437 encode < samples.txt > $(BUILD)/got.hex
	@PYTHONPATH=$(ROOT)/tools $(PYTHON) expect.py < samples.txt > $(BUILD)/want.hex
	@if diff -u $(BUILD)/want.hex $(BUILD)/got.hex > $(BUILD)/diff.txt; then \
	  echo "cp437: $$(wc -l < samples.txt) samples match tools/pit100.py"; \
	else \
	  echo "cp437: MISMATCH against tools/pit100.py"; \
	  paste -d'\t' samples.txt $(BUILD)/want.hex $(BUILD)/got.hex \
	    | awk -F'\t' '$$2 != $$3 { printf "  %-22s want %s\n  %-22s got  %s\n", $$1, $$2, "", $$3 }'; \
	  exit 1; \
	fi

clean:
	rm -rf $(BUILD)

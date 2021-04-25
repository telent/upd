default: src/upscript

LOADLIBES=-llua -lm

TESTS=$(wildcard tests/*-test.fnl)
test: $(TESTS:%.fnl=%.run)
	echo $^ $(TESTS)

tests/%-test.run: tests/%-test.fnl
	echo LUA_PATH="$(LUA_PATH)"
	./src/upscript $(FENNEL_LUA)  --add-fennel-path ./scripts/?.fnl $<
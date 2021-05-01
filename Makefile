default: src/upscript

src/upscript: src/upscript.c src/netlink.c src/netlink.h

LOADLIBES=-llua -lm
CFLAGS+=-Wall

TESTS=$(wildcard tests/*-test.fnl)
test: $(TESTS:%.fnl=%.run)
	echo $^ $(TESTS)

tests/%-test.run: tests/%-test.fnl
	echo LUA_PATH="$(LUA_PATH)"
	./src/upscript $(FENNEL_LUA)  --add-fennel-path ./scripts/?.fnl $<

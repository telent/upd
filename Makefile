default: src/upscript

SOURCES=src/upscript.c src/netlink.c src/netlink.h src/exports.c src/exports.h

src/upscript: $(SOURCES)

indent:
	astyle --style=attach $(SOURCES)

LOADLIBES=-llua -lm
CFLAGS+=-Wall

TESTS=$(wildcard tests/*-test.fnl) $(wildcard tests/*-test.rb)
test: $(TESTS:%.fnl=%.run) $(TESTS:%.rb=%.run)

tests/%-test.run: tests/%-test.fnl
	@# echo LUA_PATH="$(LUA_PATH)"
	./src/upscript $(FENNEL_LUA)  --add-fennel-path ./scripts/?.fnl $<

tests/%-test.run: tests/%-test.rb
	ruby $<

clean:
	-rm tests/netlink-capture src/upscript src/*.o

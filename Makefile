.PHONY: all clean install uninstall

BINDIR ?= /usr/bin

all:
clean:

install:
	install ./py ./pysession -t $(BINDIR)

uninstall:
	rm -rf $(BINDIR)/py $(BINDIR)/pysession

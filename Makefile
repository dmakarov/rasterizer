all: src.MAKESUBDIR

%.MAKESUBDIR:
	@$(MAKE) -s -C $*

clean:
	@$(MAKE) -s -C src clean

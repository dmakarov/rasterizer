all: src.MAKESUBDIR

%.MAKESUBDIR:
	@$(MAKE) -s -C $*

-include mk/conf.mk

ifndef CONFIG_STATUS
doConfigure: 
	$(error Configuration file not defined.  Please run make -f mk/configure.mk)
	#@make --no-print-directory -f ../mk/configure.mk
else
ifeq ($(CONFIG_STATUS),INVALID)
doConfigure:
	$(error Invalid Configuration file.  Please run make -f mk/configure.mk)
	#@make --no-print-directory -f mk/configure.mk
endif
endif

default:
	@make -s -C lib default
	@make -s -C engine

.PHONY: tools engine dedicated docs clean

tools:
	@make -s -C tools

dedicated:
	@make -s -C lib
	@make -s -C engine dedicated

engine:
	@make -s -C lib 
	@make -s -C engine engine

html_docs:
# 	Assumes Doxygen is in your path
	@doxygen doc/doxygen/html/doxygen.html.cfg

php_docs:
#  Internal gg use only
# 	Assumes Doxygen is in your path
	@doxygen doc/doxygen/php/doxygen.php.cfg

all: default tools

clean:
	@make -s -C engine clean
	@make -s -C lib clean
	@make -s -C tools clean

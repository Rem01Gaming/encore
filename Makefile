version := $(shell git rev-list HEAD --count)

all:
	sed -i "s/versionCode=.*/versionCode=$(version)/" ./module/module.prop
	clang -Wall -Wextra -O2 -o ./module/system/bin/encore-service ./src/encore-service.c
	strip -s ./module/system/bin/encore-service
	cp LICENSE ./module
	bash ./gamelist_compile.sh
	cd module && zip -r9 ../encore-$(shell uname -m)-$(version).zip * -x *placeholder
	rm module/LICENSE module/gamelist.txt module/system/bin/encore-service
	sed -i "s/versionCode=.*/versionCode=/" ./module/module.prop

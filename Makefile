version := "v1.1"
version_code := $(shell git rev-list HEAD --count)
gitsha1 := $(shell git rev-parse --short HEAD)

all:
	sed -i "s/version=.*/version=$(version) (GIT@$(gitsha1))/" ./module/module.prop
	sed -i "s/versionCode=.*/versionCode=$(version_code)/" ./module/module.prop
	clang -Wall -Wextra -O2 -o ./module/system/bin/encore-service ./src/encore-service.c
	strip -s ./module/system/bin/encore-service
	cp LICENSE ./module
	bash ./gamelist_compile.sh
	cd module && zip -r9 ../encore-$(shell uname -m)-$(version_code).zip * -x *placeholder
	rm module/LICENSE module/gamelist.txt module/system/bin/encore-service
	sed -i "s/version=.*/version=/" ./module/module.prop
	sed -i "s/versionCode=.*/versionCode=/" ./module/module.prop

prettier:
	clang-format -style=Google -i src/encore-service.c
	shfmt -l -w . module/system/bin/*
	cat gamelist.txt | sort -V >gamelist.tmp
	rm gamelist.txt
	mv gamelist.tmp gamelist.txt

version := $(shell git rev-list HEAD --count)

aarch64:
	sed -i "s/versionCode=.*/versionCode=$(version)/" ./module/module.prop
	clang -Wall -Wextra -pedantic -mllvm -polly -mllvm -polly-parallel -O3 -o ./module/system/bin/encore-service ./src/encore-service.c
	cp LICENSE gamelist.txt ./module
	cd module && zip -r9 ../encore-aarch64-$(version).zip * -x *placeholder
	rm module/LICENSE module/gamelist.txt module/system/bin/encore-service
	sed -i "s/versionCode=.*/versionCode=/" ./module/module.prop

arm:
	sed -i "s/versionCode=.*/versionCode=$(version)/" ./module/module.prop
	clang --target=armv7a-linux-androideabi -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -Wall -Wextra -pedantic -mllvm -polly -mllvm -polly-parallel -O3 -o ./module/system/bin/encore-service ./src/encore-service.c
	cp LICENSE gamelist.txt ./module
	cd module && zip -r9 ../encore-arm-$(version).zip * -x *placeholder
	rm module/LICENSE module/gamelist.txt module/system/bin/encore-service
	sed -i "s/versionCode=.*/versionCode=/" ./module/module.prop

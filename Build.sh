#!/bin/bash
ARGS=`getopt -o ab:cdkpt:vh -l help,out:,ck -- "$@"`
if [ $? != 0 ]; then
    echo "Terminating..."
    exit 1
fi
eval set -- "$ARGS"

commandType=
preClean=n
installAfterCompile=n
outPath=
logCtrl=n
buildAllImg=none
compileAll=n
clearKernel=n
compileKernel=n
copyTargetDir=n
#^(\w+?)=.*?$
#echo \1=\$\1
showAllVarExcute(){
	echo commandType=$commandType
	echo preClean=$preClean
	echo installAfterCompile=$installAfterCompile
	echo outPath=$outPath
	echo V=$logCtrl
	echo buildAllImg=$buildAllImg
	echo compileAll=$compileAll
	echo compileKernel=$compileKernel
	echo copyTargetDir=$copyTargetDir	
}
while true
do
	case "$1" in
		-a) compileAll=y
			shift;;
		-b) buildAllImg=$2
			shift 2;;			
		-c) preClean=y
			shift;;
		-k) compileKernel=y
			shift;;		
		-p) installAfterCompile=y
			shift;;	
		-t) commandType=$2
			shift 2;;
		-d) copyTargetDir=y
			shift;;				
		-v) V=s
			shift;;			
		--ck) clearKernel=y
			shift;;				
		--out) outPath=$2
			shift 2;;
		--help|-h)
			echo "  ${0##*/} help:"
			echo "		 -a:全部编译"
			echo "		 -b:[rec]:生成不带备份固件的 img;[nand_128]:生成带128k partition nand falsh 备份固件的 img;[nor_64]:生成带64k partition nor falsh 备份固件的 img"
			echo "		 -c:在编译包之前，先执行一遍 这个包的 clean"
			echo "		 -c:单独编译内核并打包"
			echo "		 -p:编译部分包并打包"
			echo "		 -t[compileCommand]:指定编译命令"
			echo "		 -d:拷贝当前包编译生成的根文件系统映射目录到nfs/`whoami`"
			echo "		 -v:显示编译log"
			echo "		 --ck:单独清理内核"	
			exit 1;;
		--) shift ;break ;;
		*) 	echo "Internal error!"
			exit 1;;
	esac
done

showAllVarExcute

for i in "$@"
do
	if [ $preClean = y ];then
		make pkg=$i cmd=sclean
	fi
	
	make pkg=$i cmd=$commandType showLog=$logCtrl || exit -1;
	
	if [ $installAfterCompile = y ];then
		make pkg=$i cmd=install
	fi
	
done






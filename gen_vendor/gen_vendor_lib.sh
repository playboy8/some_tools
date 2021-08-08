#!/bin/bash
out="gen_vendor_out.txt"


#注意：需要保证github源都有权限下载，且是以zip 格式下载数据

declare -A string_array
string_array["glog"]="https://codeload.github.com/google/glog/zip/refs/heads/master"
#string_array["boost"]="https://codeload.github.com/boostorg/boost/zip/refs/heads/master"

#编译及生成依赖文件命令
#编译之后在编译指令开始的目录生成 XXX_inc 和 XXX_lib 目录
declare -A cmd_array
cmd_array["glog"]="mkdir -p  _cmake && cd _cmake && cmake .. && make -j4 && cp -rf glog/*  ../glog_inc && cp -f libglog.so*  ../glog_lib && cd .. "
cmd_array["boost"]=""


#用于记录创建最终编译的目录
declare -A build_dirs

echo ${!string_array[@]}


echo ${string_array["glog"]}
echo ${string_array["boost"]}

echo  ----------

date=`date +"%Y-%m-%d"`
echo $date
folder=vendor
curr_dir=`pwd`
temp_dir=""

if [ ! -e "/usr/bin/wget" ]; then
	echo wget not installed , exit!
	exit
fi
if [ ! -e "/usr/bin/unzip" ]; then
	echo unzip not installed , exit!
	exit
fi

mkdir -p $folder
echo " Gen vendor file @ $date	" > ${curr_dir}/${folder}/$out

for elem in "${!string_array[@]}"
do 
	cd $curr_dir
	echo "正在下载: $elem 	源:  ${string_array[$elem]}  “"
	if [ ! -d  $folder/$elem ]; then
	rm -rf   $folder/$elem 
	mkdir -p $folder/$elem
	fi

	if [ ! -f   $folder/$elem/$elem-$date ]; then
	wget -O  $folder/$elem/$elem-$date    	${string_array[$elem]}
	fi 
	if [ ! -d  $folder/$elem/_$elem-$date ]; then
	unzip -o $folder/$elem/$elem-$date  -d  $folder/$elem/_$elem-$date
	fi
	cd $folder/$elem/_$elem-$date	

	count=1
	while [ true  ]
	do
		filenum=`ls -l |grep "^-"|wc -l`
		dirnum=`ls -l |grep "^d"|wc -l`
		let "count=$filenum+$dirnum"
		#echo $count
		if [ $count -eq 1 ]; then
			dir=`ls`
		#	echo 进入目录 $dir
			cd $dir
			temp_dir=`pwd`
		#	echo $temp_dir
		else
			break;
		fi

	done

	echo "Found root dir:$elem ---  $temp_dir"	
	build_dirs[$elem]=$temp_dir	

	echo 11111111111: ${cmd_array[$elem]}

	# creat dst folder , must be same with cmd array
	mkdir ${elem}_inc ${elem}_lib  

	eval	${cmd_array[$elem]}
	#mkdir -p  ${curr_dir}/${folder}/$elem/inc   ${curr_dir}/${folder}/$elem/lib 	
	#cp -rf ./${elem}_inc/*  ${curr_dir}/${folder}/$elem/inc
	#cp -rf ./${elem}_lib/*  ${curr_dir}/${folder}/$elem/lib
	rm -f   ${curr_dir}/${folder}/$elem/inc
	rm -f   ${curr_dir}/${folder}/$elem/lib
	ln -s $temp_dir/${elem}_inc  ${curr_dir}/${folder}/$elem/inc
	ln -s $temp_dir/${elem}_lib  ${curr_dir}/${folder}/$elem/lib

	check_dir=$temp_dir/${elem}_lib
		
	check_num=`ls ${check_dir}  -l |grep "^-"|wc -l`

	if [ $check_num -gt 0 ]; then 
	echo "$elem inc:	${curr_dir}/${folder}/$elem/inc" >> ${curr_dir}/${folder}/$out
	echo "$elem lib:	${curr_dir}/${folder}/$elem/lib" >> ${curr_dir}/${folder}/$out
	else
	echo "$elem inc: Fail " >> ${curr_dir}/${folder}/$out
	echo "$elem inc: Fail " >> ${curr_dir}/${folder}/$out
	fi

done

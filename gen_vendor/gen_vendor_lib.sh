#!/bin/bash
out="gen_vendor_out.txt"


# 填写 源码包下载地址 
#注意：需要保证github源都有权限下载，且是以zip 格式下载数据

declare -A string_array

#填写 编译及生成依赖文件命令
#编译之后在编译指令开始的目录生成 XXX_inc 和 XXX_lib 目录
declare -A cmd_array


string_array["glog"]="https://codeload.github.com/google/glog/zip/refs/heads/master"
cmd_array["glog"]="mkdir -p  _cmake && cd _cmake && cmake .. && make -j4 && cp -rf glog/*  ../glog_inc && cp -f libglog.so*  ../glog_lib && cd .. "

string_array["SQLiteCpp"]="https://codeload.github.com/playboy8/SQLiteCpp/zip/refs/heads/master"
cmd_array["SQLiteCpp"]="mkdir -p _cmake && cd _cmake && cmake -DSQLITECPP_BUILD_EXAMPLES=ON -DSQLITECPP_BUILD_TESTS=OFF .. && cmake --build .  &&  cd ..  && cp include/* SQLiteCpp_inc  && cp -rf  sqlite3 SQLiteCpp_inc/ && cp -rf _cmake/libSQLiteCpp.a  _cmake/sqlit3/libsqlite3.a SQLiteCpp_lib/ "

#boost 官网包 能编译通过 
string_array["boost"]="https://jfrog-prod-usw2-shared-oregon-main.s3.amazonaws.com/aol-boostorg/filestore/1f/1f7c8b668cba2ba7ab05f2fd5ba97d449a470fbb?x-jf-traceId=157f06ee45e44d23&response-content-disposition=attachment%3Bfilename%3D%22boost_1_77_0.zip%22&response-content-type=application%2Fzip&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Date=20210827T170053Z&X-Amz-SignedHeaders=host&X-Amz-Expires=60&X-Amz-Credential=AKIASG3IHPL63WBBRCUD%2F20210827%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Signature=32df8d8314c1f77b6acba594419e337b288d9b57c33b18c5d99439b9e7bd021c"
cmd_array["boost"]="mkdir -p boost_out && ./bootstrap.sh --prefix=./boost_out --with-python=python3 && ./b2 install -j 8 --build-type=complete --layout=tagged && cp -rf boost_out/include/* boost_inc/ && cp -rf boost_out/lib/* boost_lib/ "

string_array["rapid_json"]="https://codeload.github.com/Tencent/rapidjson/zip/refs/heads/master"
cmd_array["rapid_json"]="cp -rf include/rapidjson/*  rapid_json_inc "

string_array["protobuf"]="https://codeload.github.com/protocolbuffers/protobuf/zip/refs/heads/master"
cmd_array["protobuf"]=" rm -rf  protobuf_ ; mkdir protobuf_ &&   cmake -DCMAKE_INSTALL_PREFIX=./protobuf_  -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=ON cmake  && make && make install && cp -rf protobuf_/include/* protobuf_inc && cp -rf protobuf_/lib64/* protobuf_lib && cp -rf protobuf_/bin protobuf_lib  "

string_array["hiredis"]="https://codeload.github.com/redis/hiredis/zip/refs/heads/master"
cmd_array["hiredis"]="mkdir -p hiredis_ && make PREFIX=./hiredis_ && make PREFIX=./hiredis_  install && cp -rf hiredis_/*  hiredis_inc"

string_array["redis-plusplus"]="https://codeload.github.com/sewenew/redis-plus-plus/zip/refs/heads/master"
cmd_array["redis-plusplus"]="mkdir redis_cpp_ && mkdir cmake_ && cd cmake_ && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=../redis_cpp_/redis/hiredis -DCMAKE_INSTALL_PREFIX=../redis_cpp_/redis/   && cp -rf ../redis_cpp_/redis ../redis-plusplus_inc"

#string_array["kdb"]=""
#cmd_array["kdb"]=""






#用于记录创建最终编译的目录
declare -A build_dirs

echo ${!string_array[@]}


echo ${string_array["glog"]}
echo ${string_array["boost"]}

echo  ----------

date=`date +"%Y-%m"`
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
echo "[ Gen vendor file @  `date +"%Y-%m-%d %H:%M:%S"` 	]" > ${curr_dir}/${folder}/$out

for elem in "${!string_array[@]}"
do 
	cd $curr_dir
	echo "正在下载: $elem 	源:  ${string_array[$elem]}  “"
	if [  ${string_array[$elem]} == "" ]; then
		continue
	fi
	if [ ! -d  $folder/$elem ]; then
	rm -rf   $folder/$elem 
	mkdir -p $folder/$elem
	fi

	if [ ! -f   $folder/$elem/$elem-$date ]; then
	rm -rf $folder/$elem/$elem-*
	wget -O  $folder/$elem/$elem-$date    	${string_array[$elem]}
	fi 
	if [ ! -d  $folder/$elem/_$elem-$date ]; then
	rm -rf $folder/$elem/_$elem-*
	unzip -o $folder/$elem/$elem-$date  -d  $folder/$elem/_$elem-$date || rm -rf $folder/$elem/$elem-$date  $folder/$elem/_$elem-$date 
	fi

	if [ ! -d $folder/$elem/_$elem-$date ]; then
		echo "$elem ： 解压源码包失败 ，退出 " >>  ${curr_dir}/${folder}/$out
		continue 
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

	echo "开始编译项目 $elem .... "
       	echo "	${cmd_array[$elem]}  "

	# creat dst folder , must be same with cmd array
	mkdir -p  ${elem}_inc ${elem}_lib  

	eval	${cmd_array[$elem]}
 
	rm -f   ${curr_dir}/${folder}/$elem/inc
	rm -f   ${curr_dir}/${folder}/$elem/lib
	ln -s $temp_dir/${elem}_inc  ${curr_dir}/${folder}/$elem/inc
	ln -s $temp_dir/${elem}_lib  ${curr_dir}/${folder}/$elem/lib

	# 因为有的库只有头文件
	check_dir=$temp_dir/${elem}_inc
		
	check_num=`ls ${check_dir}  -l |wc -l`

	if [ $check_num -gt 0 ]; then 
	echo "$elem inc:	${curr_dir}/${folder}/$elem/inc" >> ${curr_dir}/${folder}/$out
	echo "$elem lib:	${curr_dir}/${folder}/$elem/lib" >> ${curr_dir}/${folder}/$out

#	mkdir -p ${curr_dir}/${folder}_$date
#	cp -rf ${curr_dir}/${folder}/$elem/inc ${curr_dir}/${folder}_$date/
#	cp -rf ${curr_dir}/${folder}/$elem/inc ${curr_dir}/${folder}_$date/

	else
	echo "$elem inc: Fail " >> ${curr_dir}/${folder}/$out
	echo "$elem inc: Fail " >> ${curr_dir}/${folder}/$out
	fi

done

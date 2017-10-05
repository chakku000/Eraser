#/bin/sh
# コンパイル対象の拡張子
ext="*.c"

#特定拡張子のファイルをコンパイルする
compile(){
    for fName in `find . -name "$ext"`
    do
        echo "compile " $fName
        name=`echo $fName | sed -e "s/\.c//" | sed -e "s/\.\///"`
        gcc -pthread -o $name $fName
    done
}

echo "#########  Compile Start ##########"
compile
echo "#########   Compile END  ##########"

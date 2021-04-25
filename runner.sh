
cd CAT_lib/misc;

filename="sample.c"
ex=$(echo "$filename" | cut -f 1 -d '.')

clang -S -emit-llvm $filename
clang -emit-llvm -c $filename
noelle-norm "$ex".bc -o "$ex".bc; 
noelle-load -load ~/CAT/lib/CAT.so -CAT "$ex".bc >/dev/null; 
llc -filetype=obj "$ex".bc;
clang "$ex".o CAT.c;
./a.out;

cd ../..;
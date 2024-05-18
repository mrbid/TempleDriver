mkdir -p high
for file in ply/*.ply; do
    echo $file
	cd ply
	./../pto $(basename "$file")
	tmp=$(basename "$file" .ply)
	nasm -f elf64 $tmp.asm -o ../high/$tmp.o
	rm $tmp.asm
	cd ..
	mv -f ply/$tmp.h high/$tmp.h
done;

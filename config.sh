#! /bin/bash

huge_mnt_path="/mnt/hugepages"
objs_path="objs"
HUGEPGSZ=`cat /proc/meminfo | grep Hugepagesize | cut -d : -f 2 |tr -d ' '`

#test if dir exit
if [ ! -x $huge_mnt_path ]
then
	mkdir -p $huge_mnt_path
fi

if [ ! -x $objs_path ]
then
	mkdir $objs_path
fi

mount -t hugetlbfs nodev "$huge_mnt_path"
echo "Size of current system's hugepage is ${HUGEPGSZ}"
echo "Input number of pages:"
read pages 
echo $pages > /proc/sys/vm/nr_hugepages

for dir in "lib" "include"
do
    mkdir ${PREFIX}/${dir}
    cp -r ${SRC_DIR}/${dir}/* -t ${PREFIX}/${dir}
done
exit 0

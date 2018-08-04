reset;
rm -rf build/;
python3.6 setup_uvmodule.py build;
cp -rf build/lib.linux-x86_64-3.6/* .;
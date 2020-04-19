# Common ci routines.


function repack() {
    # Repack input tarball using external, gnu tar.
    # $1: tarball to repack
    # $2: Optional plain file added to tarball root dir

    local tmpdir=repack.$$
    rm -rf $tmpdir && mkdir $tmpdir
    tar -C $tmpdir -xf $1
    if [ -n "$2" ]; then cp $2 $tmpdir; fi
    tar -C $tmpdir -czf $1 .
    rm -rf $tmpdir
}



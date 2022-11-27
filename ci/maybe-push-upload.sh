#!/bin/bash
#
# Read build.conf.sh and conditionally run upload and git_push scripts.

here=$(cd $(dirname $0); pwd -P)
source $here/../build-conf.rc

if [ ! -d /build-$OCPN_TARGET ]; then exit 0; fi   # no build available

if [ "$oldstable_upload" = "true" ]; then
    sh -c "cd /build-$OCPN_TARGET; /bin/bash < upload.sh"
fi
if [ "$oldstable_git_push" = "true" ]; then
    sh -c "ci/git-push.sh /build-$OCPN_TARGET"
fi

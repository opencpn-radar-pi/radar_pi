#!/usr/bin/env bash
#
# Temporary hack to handle .uploadrc  and local-build.rc when used for
# PYTHONPATH and git-push
set -x
python3 --version
which python3
env
here=$(cd $(dirname $0); pwd -P)
if [ -f ~/.config/local-build.rc ]; then source ~/.config/local-build.rc; fi
if [ -f ~/.uploadrc ]; then source ~/.uploadrc; fi
python3 --version
cd $here
python3 git-push $@

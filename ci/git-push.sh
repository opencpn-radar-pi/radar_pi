#!/bin/bash
#
# Temporary hack to handle .uploadrc when used for PYTHONPATH and git-push

here=$(dirname $(realpath -e $0))
if [ -f ~/.uploadrc ]; then source ~/.uploadrc; fi
cd $here
python git-push


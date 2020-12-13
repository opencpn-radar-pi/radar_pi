#!/usr/bin/env bash


STABLE_REPO='opencpn-radar-pi/opencpn-radar-pi-prod'
UNSTABLE_REPO='opencpn-radar-pi/opencpn-radar-pi-beta'

if [ -z "$CLOUDSMITH_API_KEY" ]; then
    echo 'Cannot deploy to cloudsmith, missing $CLOUDSMITH_API_KEY'
    exit 1
fi

echo "Using \$CLOUDSMITH_API_KEY: ${CLOUDSMITH_API_KEY:0:4}..."


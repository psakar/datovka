#!/usr/bin/env sh

HAVE_ERROR="false"

echo_error() {
	HAVE_ERROR="true"
	RED='\033[1;31m' # Set colour
	NC='\033[0m' # No colour
	echo -e "${RED}$1${NC}"
}

echo_success() {
	GREEN='\033[1;32m' # Set colour
	NC='\033[0m' # No colour
	echo -e ${GREEN}$1${NC}
}

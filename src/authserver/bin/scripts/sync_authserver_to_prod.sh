#!/bin/sh

BASEDIR="$(dirname $(realpath $0))"
RSYNC_PROGRAM="rsync"

SRC_PATH="$(dirname "$BASEDIR")"
DEST_PATH="~/authserver-dist"
SRC_FILE_LIST="${SRC_PATH}/authserver_files.txt"
EXCLUDE="${SRC_PATH}/authserver_exclude.txt"

SSH_PORT=53122
CREDENTIAL="~/credentials/MySSHKey_Virginia.pem"
USER="ubuntu"
HOST="54.89.2.82"

sync_files() {
	echo "Start synchronizing files..."

	${RSYNC_PROGRAM} -vrRz -e "ssh -p ${SSH_PORT} -i ${CREDENTIAL}" --checksum --exclude-from="${EXCLUDE}" --files-from=${SRC_FILE_LIST} ${SRC_PATH} ${USER}@${HOST}:${DEST_PATH}
}

read -p "Do you want to sync files to ${USER}@${HOST}:${DEST_PATH}? [Y/n]" choice
case "$choice" in
	 y|Y)
		sync_files
		;;
	*)
		;;
esac


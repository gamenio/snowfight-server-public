#!/bin/sh

BASEDIR="$(dirname $(realpath $0))"
RSYNC_PROGRAM="rsync"

FILE_LIST="${BASEDIR}/worldserver_files.txt"
EXCLUDE="${BASEDIR}/worldserver_exclude.txt"

TEMP_DIR="${BASEDIR}/worldserver-temp"

SRC_PATH="~/worldserver-18403"
SRC_SSH_PORT=53122
SRC_CREDENTIAL="~/credentials/MySSHKey_Virginia.pem"
SRC_USER="ubuntu"
SRC_HOST="54.89.2.82"

DEST_PATH="~/worldserver-dist"
DEST_SSH_PORT=53122
DEST_CREDENTIAL="~/credentials/MySSHKey_aliyun.pem"
DEST_USER="ubuntu"
DEST_HOST="39.106.120.21"


echo "Start synchronizing files from ${SRC_USER}@${SRC_HOST}..."

${RSYNC_PROGRAM} -vrRz -e "ssh -p ${SRC_SSH_PORT} -i ${SRC_CREDENTIAL}" --checksum --exclude-from="${EXCLUDE}" --files-from=${FILE_LIST} ${SRC_USER}@${SRC_HOST}:${SRC_PATH} ${TEMP_DIR}

if [ $? -ne 0 ]; then
	exit 1
fi

${RSYNC_PROGRAM} -vrRz -e "ssh -p ${DEST_SSH_PORT} -i ${DEST_CREDENTIAL}" --exclude-from="${EXCLUDE}" --files-from=${FILE_LIST} ${TEMP_DIR} ${DEST_USER}@${DEST_HOST}:${DEST_PATH} 
rm -r "${TEMP_DIR}"

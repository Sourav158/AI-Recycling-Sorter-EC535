#!/bin/bash
# EC535 rootfs.img Rebuilding Script
# Provided with stock-rootfs.tar.gz
ROOTFS_SIZE=300

# Ensure WORKSPACE is set
if [ -z "$WORKSPACE" ]; then
    echo "Error: WORKSPACE variable must be set"
    exit 1
fi

# Ensure mountpoint is empty
if [ "$(ls -A /tmp/$USER-mnt)" ]; then
    echo "Error: /tmp/$USER-mnt must be empty in order to be used as a mountpoint for your rootfs.img."
    echo "Make sure a filesystem is not already mounted there, and then run 'rm -rf /tmp/$USER-mnt'."
    exit 1
fi


# Don't do anything unless a rootfs folder exists in the working dir
if [ -d "${WORKSPACE}/rootfs" ]; then
    # Delete rootfs.img if it exists
    if [ -e "rootfs.img" ]; then
        echo "Deleting old rootfs.img..."
        rm rootfs.img
    fi

    # Create a blank 128MB disk image formatted as ext4
    echo "Creating blank ${ROOTFS_SIZE}MB rootfs.img..."
    dd if=/dev/zero of=rootfs.img bs=1M count=$ROOTFS_SIZE
    echo "Formatting rootfs.img as ext4..."
    yes | mkfs.ext4 -L rootfs rootfs.img

    # Mount disk image and copy in files
    echo "Mounting rootfs.img on /tmp/${USER}-mnt..."
    mkdir -p /tmp/$USER-mnt
    guestmount -a rootfs.img -m /dev/sda /tmp/$USER-mnt
    echo "Copying files..."
    cp -a --no-preserve=ownership $WORKSPACE/rootfs/* /tmp/$USER-mnt/
    echo "Unmounting rootfs.img..."
    guestunmount /tmp/$USER-mnt/

    # List files so user can confirm copy completed successfully
    virt-ls -la rootfs.img -m /dev/sda /
    echo "All done!"
else
    echo "Error: rootfs does not exist in your WORKSPACE directory"
fi

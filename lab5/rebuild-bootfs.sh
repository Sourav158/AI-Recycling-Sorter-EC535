#!/bin/bash
# EC535 bootfs.img Rebuilding Script
# Provided with stock-bootfs.tar.gz
BOOTFS_SIZE=62

# Ensure WORKSPACE is set
if [ -z "$WORKSPACE" ]; then
    echo "Error: WORKSPACE variable must be set"
    exit 1
fi

# Ensure mountpoint is empty
if [ "$(ls -A /tmp/$USER-mnt)" ]; then
    echo "Error: /tmp/$USER-mnt must be empty in order to be used as a mountpoint for your bootfs.img."
    echo "Make sure a filesystem is not already mounted there, and then run 'rm -rf /tmp/$USER-mnt'."
    exit 1
fi

# Don't do anything unless a bootfs folder exists in the working dir
if [ -d "${WORKSPACE}/bootfs" ]; then
    # Delete bootfs.img if it exists
    if [ -e "bootfs.img" ]; then
        echo "Deleting old bootfs.img..."
        rm bootfs.img
    fi

    echo Initializing new bootfs.img...
    # Create the FAT16 filesystem and guestmount it
    dd if=/dev/zero of=bootfs.img bs=1M count=$BOOTFS_SIZE
    mkfs.vfat bootfs.img

    # Mount disk image and copy in files
    echo "Mounting bootfs.img on /tmp/${USER}-mnt..."
    mkdir -p /tmp/$USER-mnt
    guestmount -a bootfs.img -m /dev/sda /tmp/$USER-mnt
    echo "Copying files..."
    cp -a --no-preserve=ownership $WORKSPACE/bootfs/* /tmp/$USER-mnt/
    echo "Unmounting bootfs.img..."
    guestunmount /tmp/$USER-mnt/

    # List files so user can confirm copy completed successfully
    virt-ls -la bootfs.img -m /dev/sda /
    echo "All done!"
else
    echo "Error: bootfs does not exist in your WORKSPACE directory"
fi


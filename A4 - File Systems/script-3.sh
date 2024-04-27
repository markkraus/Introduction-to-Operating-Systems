#!/bin/bash

# testing mkdir, mknod, rmdir, unlink, and readdir

# Function called whenever a test is passed. Increments num_tests_passed
pass() {
  echo PASS
}

# Function is called whenever a test fails.
fail() {
  echo FAIL
  exit 1
}

MOUNT=testmount

if [ ! -f "./cs1550" ]; then echo "Compilation Errors"; exit 0; fi

sleep 3

declare -i n=0

echo -ne "for i in {0..9}; do\n   mkdir ${MOUNT}/dir_i\ndone\n"

for i in {0..9}; do
  echo "mkdir ${MOUNT}/dir_${i}"
  err=$((mkdir ${MOUNT}/dir_${i}) 2>&1)
  echo $err
  if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
  then
    echo "Program crashed";
    exit 1;
  fi
done

echo -ne "ls -al ${MOUNT}\n"
err=$((ls -al ${MOUNT} | sed 1d | awk '{print $1, $2, $3, $4, $5, $9}' > output-ls2.txt) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo -ne "for i in {0..9}; do\n   echo -ne \"\" > ${MOUNT}/dir_0/file_i.txt\ndone\n"

for i in {0..9}; do
  echo "echo -ne \"\" > ${MOUNT}/dir_0/file_i.txt"
  err=$((echo -ne "" > ${MOUNT}/dir_0/file_${i}.txt) 2>&1)
  echo $err
  if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
  then
    echo "Program crashed";
    exit 1;
  fi
done

echo -ne "ls -al ${MOUNT}/dir_0\n"
err=$((ls -al ${MOUNT}/dir_0 | sed 1d | awk '{print $1, $2, $3, $4, $5, $9}' > output-ls3.txt) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo -ne "for i in {1..9}; do\n   rmdir ${MOUNT}/dir_i\ndone\n"
for i in {1..9}; do
  echo "rmdir ${MOUNT}/dir_${i}"
  err=$((rmdir ${MOUNT}/dir_${i}) 2>&1)
  echo $err
  if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
  then
    echo "Program crashed";
    exit 1;
  fi
done

echo -ne "ls -al ${MOUNT}\n"
err=$((ls -al ${MOUNT} | sed 1d | awk '{print $1, $2, $3, $4, $5, $9}' > output-ls5.txt) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo -ne "rmdir ${MOUNT}/dir_0\n"
err=$((rmdir ${MOUNT}/dir_0) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo -ne "for i in {0..9}; do\n  rm ${MOUNT}/dir_0/file_i.txt\ndone\n"

for i in {0..9}; do
  echo "rm ${MOUNT}/dir_0/file_i.txt"
  err=$((rm ${MOUNT}/dir_0/file_${i}.txt) 2>&1)
  echo $err
  if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
  then
    echo "Program crashed";
    exit 1;
  fi
done

echo -ne "ls -al ${MOUNT}/dir_0\n"
err=$((ls -al ${MOUNT}/dir_0 | sed 1d | awk '{print $1, $2, $3, $4, $5, $9}' > output-ls6.txt) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

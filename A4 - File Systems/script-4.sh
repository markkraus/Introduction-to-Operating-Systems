#!/bin/bash

# mkdir, mknod, and trunc testing

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


err=$((mkdir ${MOUNT}/dir0) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi


echo "echo -ne \"\" > ${MOUNT}/dir0/12345678.123"
err=$((echo -ne "" > ${MOUNT}/dir0/12345678.123) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

if [ -f "${MOUNT}/dir0/12345678.123" ]; then echo "PASS 0"; else fail; fi

echo "Making sure size is zero"
size=$(wc -c <"${MOUNT}/dir0/12345678.123")
if [ $size -eq 0 ]; then
    echo "PASS 1";
else
    fail;
fi

echo "Handles invalid names or attempting to create a file in root..."
echo "echo -n \"\" > ${MOUNT}/dir0/123456789.1234"
err=$((echo -n "" > ${MOUNT}/dir0/123456789.1234) 2>&1)
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

if [ -f "${MOUNT}/dir0/123456789.1234" ]; then fail; else echo "PASS 2"; fi


echo "echo -n \"\" > ${MOUNT}/file0.dat"
err=$((echo -n "" > ${MOUNT}/file0.dat) 2>&1)
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

if [ ! -f "${MOUNT}/dir0/12345678.123" ]; then echo "Program Crashed"; exit 0; fi
if [ -f "${MOUNT}/file0.dat" ]; then fail; else echo "PASS 3"; fi


echo "Reports no space when subdirectory has hit the limit for files..."
for((i=0;i<23;i++))
do
      echo -ne ">> echo -ne \"\" >${MOUNT}/dir0/file$i.dat"
      err=$((echo -ne "" > ${MOUNT}/dir0/file$i.dat) 2>&1)
      echo $err
      if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
      then
        echo "Program crashed";
        exit 1;
      fi
      if [ ! -f "${MOUNT}/dir0/file$i.dat" ]; then echo "Program Crashed"; exit 0; fi
done

echo ">> echo -ne \"\" >${MOUNT}/dir0/file23.dat"
err=$((echo -ne "" > ${MOUNT}/dir0/file23.dat) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi
if [ -f "${MOUNT}/dir0/file23.dat" ]; then fail; else echo "PASS 4"; fi

err=$((mkdir ${MOUNT}/dir1) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo "echo -ne \"\" > ${MOUNT}/dir1/12345678.123"
err=$((echo -ne "" > ${MOUNT}/dir1/12345678.123) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo "trunc -s 1000 ${MOUNT}/dir1/12345678.123"
err=$((trunc -s 1000 ${MOUNT}/dir1/12345678.123) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo "Making sure size is changed"
size=$(wc -c <"${MOUNT}/dir1/12345678.123")
if [ $size -eq 1000 ]; then
    echo "PASS 5";
else
    fail;
fi

echo "trunc -s 100 ${MOUNT}/dir1/12345678.123"
err=$((trunc -s 100 ${MOUNT}/dir1/12345678.123) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo "Making sure size is changed"
size=$(wc -c <"${MOUNT}/dir1/12345678.123")
if [ $size -eq 100 ]; then
    echo "PASS 6";
else
    fail;
fi

echo "trunc -s 999999999999 ${MOUNT}/dir1/12345678.123"
err=$((trunc -s 999999999999 ${MOUNT}/dir1/12345678.123) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo "Making sure size is not changed because new size is too big"
size=$(wc -c <"${MOUNT}/dir1/12345678.123")
if [ $size -eq 100 ]; then
    echo "PASS 7";
else
    fail;
fi
echo "trunc -s -10 ${MOUNT}/dir1/12345678.123"
err=$((trunc -s -10 ${MOUNT}/dir1/12345678.123) 2>&1)
echo $err
if [[ $err == *"abort"* ]] || [[ $err == *"not connected"* ]]
then
  echo "Program crashed";
  exit 1;
fi

echo "Making sure size is not changed because new size is negative"
size=$(wc -c <"${MOUNT}/dir1/12345678.123")
if [ $size -eq 100 ]; then
    echo "PASS 8";
else
    fail;
fi

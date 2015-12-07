#!/bin/bash

for i in `seq 20`;
do
  cat sample_request | nc -G 2 localhost 80 &
done

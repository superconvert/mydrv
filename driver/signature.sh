#!/bin/sh

root_path=/lib/modules/$(uname -r)/build
bin_file=${root_path}/scripts/sign-file
key_file=${root_path}/certs/signing_key.x509
cert_file=${root_path}/certs/signing_key.pem
${bin_file} sha512 ${cert_file} ${key_file} mydrv.ko

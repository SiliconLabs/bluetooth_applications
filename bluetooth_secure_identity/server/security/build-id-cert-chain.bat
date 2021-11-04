rem Root certificate section
openssl genpkey -algorithm EC -out root\root-signing-private-key.pem -pkeyopt ec_paramgen_curve:P-256 -pkeyopt ec_param_enc:named_curve
openssl pkey -in root\root-signing-private-key.pem -pubout -out root\root-signing-public-key.pem

openssl req -new -config root\root-csr.cfg -key root\root-signing-private-key.pem -out root\root-test.csr -subj "/C=US/O=Silicon Labs/CN=Root"
openssl x509 -extfile root\root-sign.cfg -addtrust anyExtendedKeyUsage -days 36500 -signkey root\root-signing-private-key.pem -req -in root\root-test.csr -out root\root-cert.pem

rem Factory certificate section
openssl genpkey -algorithm EC -out factory\factory-signing-private-key.pem -pkeyopt ec_paramgen_curve:P-256 -pkeyopt ec_param_enc:named_curve
openssl pkey -in factory\factory-signing-private-key.pem -pubout -out factory\factory-signing-public-key.pem

openssl req -new -in factory\factory-csr.cfg -key factory\factory-signing-private-key.pem -out factory\factory-test.csr -subj "/C=US/O=Silicon Labs Factory/CN=Factory"
openssl x509 -extfile factory\factory-sign.cfg -addtrust anyExtendedKeyUsage -days 36500 -req -CA root\root-cert.pem -CAkey root\root-signing-private-key.pem -CAcreateserial -in factory\factory-test.csr -out factory\factory-cert.pem
rem echo off
rem Batch certificate section
openssl genpkey -algorithm EC -out batch\batch-signing-private-key.pem -pkeyopt ec_paramgen_curve:P-256
openssl pkey -in batch\batch-signing-private-key.pem -pubout -out batch\batch-signing-public-key.pem

openssl req -new -in batch\batch-csr.cfg -batch -key batch\batch-signing-private-key.pem -out batch\batch-test.csr -subj "/C=US/O=Silicon Labs Factory/CN=Batch 1"
openssl x509 -extfile batch\batch-sign.cfg -req -addtrust anyExtendedKeyUsage -days 36500 -CA factory\factory-cert.pem -CAkey factory\factory-signing-private-key.pem -CAcreateserial -in batch\batch-test.csr -out batch\batch-cert.pem

rem device certificate section
openssl genpkey -algorithm EC -out device\device-signing-private-key.pem -pkeyopt ec_paramgen_curve:P-256
openssl pkey -in device\device-signing-private-key.pem -pubout -out device\device-signing-public-key.pem

openssl req -new -in device\device-csr.cfg -batch -key device\device-signing-private-key.pem -out device\device-test.csr -subj "/C=US/O=Silicon Labs Factory/CN=Sample Device"
openssl x509 -extfile device\device-sign.cfg -req -addtrust anyExtendedKeyUsage -days 36500 -CA batch\batch-cert.pem -CAkey batch\batch-signing-private-key.pem -CAcreateserial -in device\device-test.csr -out device\device-cert.pem
rem echo on
openssl verify -show_chain -CAfile root\root-cert.pem factory\factory-cert.pem batch\batch-cert.pem device\device-cert.pem
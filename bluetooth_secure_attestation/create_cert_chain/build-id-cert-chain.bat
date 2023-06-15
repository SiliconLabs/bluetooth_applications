@echo off 
rem create CA key
openssl genpkey -algorithm EC -out root_key.pem -pkeyopt ec_paramgen_curve:P-256 -pkeyopt ec_param_enc:named_curve
openssl ec -in root_key.pem -pubout -out root_pubkey.pem
rem create CA cert
openssl req -new -config cfg/root-csr.cfg -key root_key.pem -out root_csr.pem -subj "/CN=Root/C=US/O=Silicon Labs"
openssl x509 -extfile cfg/root-sign.cfg -addtrust anyExtendedKeyUsage -days 36500 -signkey root_key.pem -req -in root_csr.pem -out root_cert.pem

rem create factory private key
openssl genpkey -algorithm EC -out factory_key.pem -pkeyopt ec_paramgen_curve:P-256 -pkeyopt ec_param_enc:named_curve
rem generate factory public key
openssl ec -in factory_key.pem -pubout -out factory_pubkey.pem
rem generate factory signing request
openssl req -new -config cfg/factory-csr.cfg -key factory_key.pem -out factory_sign_req.pem -subj "/CN=Factory/C=US/O=Silicon Labs"
rem create factory cert
openssl x509 -extfile cfg/factory-sign.cfg -addtrust anyExtendedKeyUsage -days 36500 -req -in factory_sign_req.pem -CA root_cert.pem -CAkey root_key.pem -CAcreateserial -out factory_cert.pem

rem create batch private key
openssl genpkey -algorithm EC -out batch_key.pem -pkeyopt ec_paramgen_curve:P-256 -pkeyopt ec_param_enc:named_curve
rem generate batch public key
openssl ec -in batch_key.pem -pubout -out batch_pubkey.pem
rem generate batch signing request
openssl req -new -config cfg/batch-csr.cfg -key batch_key.pem -out batch_sign_req.pem -subj "/CN=Batch/C=US/O=Silicon Labs"
rem create batch cert
openssl x509 -extfile cfg/batch-sign.cfg -addtrust anyExtendedKeyUsage -days 36500 -req -in batch_sign_req.pem -CA factory_cert.pem -CAkey factory_key.pem -CAcreateserial -out batch_cert.pem

rem create device private key
openssl genpkey -algorithm EC -out device_key.pem -pkeyopt ec_paramgen_curve:P-256 -pkeyopt ec_param_enc:named_curve
rem generate device public key
openssl ec -in device_key.pem -pubout -out device_pubkey.pem
rem generate device signing request
openssl req -new -config cfg/device-csr.cfg -key device_key.pem -out sign_req.pem -subj "/CN=Example device/C=US/O=Silicon Labs"
rem create device cert
openssl x509 -extfile cfg/device-sign.cfg -addtrust anyExtendedKeyUsage -days 36500 -req -in sign_req.pem -CA batch_cert.pem -CAkey batch_key.pem -CAcreateserial -out device_cert.pem

rem verify chain
echo ''
echo "Verify chain"
echo ''
type batch_cert.pem factory_cert.pem root_cert.pem > ca_cert.pem
openssl verify -show_chain -CAfile ca_cert.pem  device_cert.pem
TEMPLATE = subdirs

SUBDIRS = helpz dtls_server dtls_client

dtls_server.depends = helpz
dtls_client.depends = helpz

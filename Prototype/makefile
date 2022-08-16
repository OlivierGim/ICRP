Sender.out: Sender.c functions_verify.c functions_string.c functions_args_conv.c functions_others.c functions.h
	gcc -o Sender.out Sender.c functions_verify.c functions_string.c functions_args_conv.c functions_others.c functions.h -lssl -lcrypto -lpthread

Receiver.out: Receiver.c functions_verify.c functions_string.c functions_args_conv.c functions_others.c functions.h
	gcc -o Receiver.out Receiver.c functions_verify.c functions_string.c functions_args_conv.c functions_others.c functions.h -lssl -lcrypto

finalSender.out: finalSender.c functions_verify.c functions_string.c functions_args_conv.c functions_others.c functions.h
	gcc -Wall -o finalSender.out finalSender.c functions_args_conv.c functions_others.c functions_string.c functions_verify.c -lpthread -lssl -lcrypto

finalReceiver.out: finalReceiver.c functions_verify.c functions_string.c functions_args_conv.c functions_others.c functions.h
	gcc -Wall -o finalReceiver.out finalReceiver.c functions_args_conv.c functions_others.c functions_string.c functions_verify.c -lpthread -lssl -lcrypto

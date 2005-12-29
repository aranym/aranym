#ifndef ARANYMEXCEPTION_H
#define ARANYMEXCEPTION_H

class AranymException {
	private:
		char errMsg[256];

	public:
		AranymException(char *fmt, ...);
		virtual ~AranymException();

		char *getErrorMessage(void);
};

#endif /* ARANYMEXCEPTION_H */

#ifndef CHKUSER_H
#define CHKUSER_H

typedef enum {
	GOOD_USER,
	BAD_PASSWORD,
	BAD_USER
} UserStatus;

UserStatus CheckUserid(const char *userid, const char *password);

const char *UserAddress(const char *userid);

int DigestStatus(const char *userid);

const char *GetUserOptions(const char *game, const char *userid);

void SetUserOptions(const char *game, const char *userid, const char *option);

#endif

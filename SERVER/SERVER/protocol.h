constexpr int PORT_NUM = 4000;
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 100;
constexpr int BUF_SIZE = 1024;

constexpr int MAX_USER = 10000;
constexpr int MAX_NPC = 200000;

constexpr int W_WIDTH = 2000;
constexpr int W_HEIGHT = 2000;


//GameAnimation
enum CHARECTORSTATE {
	IDLE,
	WALK,
	ATTACK,
	CHARSTATEEND,
};
enum CHARJOB {
	WARRRIOR,
	ROBOT,
	NINJA,
	//몬스터종류
	WOMANZOMBIE,
	MANZOMBIE,
	DOG,
	DINO,
	CAT,
	CHARJOBEND
};
enum UI {
	grayhp,
	redhp,
	chat,
	UIEND

};
// Packet ID
constexpr char CS_LOGIN = 0;
constexpr char CS_MOVE = 1;
constexpr char CS_CHAT = 2;
constexpr char CS_ATTACK = 3;			// 4 방향 공격
constexpr char CS_TELEPORT = 4;			// RANDOM한 위치로 Teleport, Stress Test할 때 Hot Spot현상을 피하기 위해 구현
constexpr char CS_LOGOUT = 5;			// 클라이언트에서 정상적으로 접속을 종료하는 패킷

constexpr char SC_LOGIN_INFO = 2;
constexpr char SC_LOGIN_FAIL = 3;
constexpr char SC_ADD_OBJECT = 4;
constexpr char SC_REMOVE_OBJECT = 5;
constexpr char SC_MOVE_OBJECT = 6;
constexpr char SC_CHAT = 7;
constexpr char SC_STAT_CHANGE = 8;
constexpr char SC_ATTACK = 9;

#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	unsigned short size;
	char	type;
	char	userid[NAME_SIZE];
	char	name[NAME_SIZE];
};

struct CS_MOVE_PACKET {
	unsigned short size;
	char	type;
	char	direction;  // 0 : UP, 1 : DOWN, 2 : LEFT, 3 : RIGHT
	unsigned	move_time;
};

struct CS_CHAT_PACKET {
	unsigned short size;			// 크기가 가변이다, mess가 작으면 size도 줄이자.
	char	type;
	WCHAR	mess[CHAT_SIZE];
};

struct CS_ATTACK_PACKET {
	unsigned short size;			// 크기가 가변이다, mess가 작으면 size도 줄이자.
	char	type;
};

struct CS_TELEPORT_PACKET {			// 랜덤으로 텔레포트 하는 패킷, 동접 테스트에 필요
	unsigned short size;
	char	type;
};

struct CS_LOGOUT_PACKET {
	unsigned short size;
	char	type;
};

struct SC_LOGIN_INFO_PACKET {
	unsigned short size;
	char	type;
	char	userid[NAME_SIZE];
	int		visual;				// 종족, 성별등을 구분할 때 사용
	int		id;
	int		hp;
	int		max_hp;
	int		attack_damge;
	int		exp;
	int		level;
	short	x, y;
};

struct SC_ADD_OBJECT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		visual;				// 어떻게 생긴 OBJECT인가를 지시
	int		dir;
	int		state; //적이 가만히잇을때 추가됐는지 아니면 움직이다가 추가됏는지 확인용
	int		hp;
	int		max_hp;
	int		level;
	short	x, y;
	char	name[NAME_SIZE];
};

struct SC_REMOVE_OBJECT_PACKET {
	unsigned short size;
	char	type;
	int		id;
};

struct SC_MOVE_OBJECT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		dir;
	short	x, y;
	unsigned int move_time;
};

struct SC_CHAT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	WCHAR	mess[CHAT_SIZE];
};

struct SC_LOGIN_FAIL_PACKET {
	unsigned short size;
	char	type;
};

struct SC_ATTACK_PACKET {
	unsigned short size;
	char	type;
	int		id;
};

struct SC_STAT_CHANGE_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;

};

#pragma pack (pop)
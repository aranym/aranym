/* Joy 2001 */

class ACIA {
private:
	bool midi;
	uae_u8 status;
	uae_u8 mode;
	uae_u8 rxdata;
	uae_u8 txdata;

public:
	ACIA(bool);
	uae_u8 getStatus();
	void setMode(uae_u8 value);
	uae_u8 getData();
	void setData(uae_u8);
};

class IKBD: public ACIA {
public:
	IKBD() : ACIA(false) {};
};

class MIDI: public ACIA {
public:
	MIDI() : ACIA(true) {};
};

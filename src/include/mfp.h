/* Joy 2001 */

class MFP_Timer {
private:
	char name;
	uae_u8 control;
	uae_u8 start_data, current_data;
	bool state;

protected:
	bool MFP_Timer::isRunning();

public:
	MFP_Timer(char);
	void setControl(uae_u8);
	uae_u8 getControl();
	void setData(uae_u8);
	uae_u8 getData();
};

class USART {
public:
	uae_u8 synchar;
	uae_u8 control;
	uae_u8 rxstat;
	uae_u8 txstat;
	uae_u8 data;
};

class MFP {
private:
	uae_u8 GPIP_data;
	uae_u8 active_edge;
	uae_u8 data_direction;
	uae_u16 irq_enable;
	uae_u16 irq_pending;
	uae_u16 irq_inservice;
	uae_u16 irq_mask;
	uae_u8 irq_vector;
	MFP_Timer *tA, *tB, *tC, *tD;
	USART usart;

public:
	MFP();
	uae_u32 handleRead(uaecptr);
	void handleWrite(uaecptr, uae_u8);
};

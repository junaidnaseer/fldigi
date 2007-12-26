// modem class - base for all modems
//

#include <config.h>

#include "Config.h"
#include "modem.h"
#include "configuration.h"

#include "qrunner.h"

#include "status.h"

modem *cw_modem = 0;
modem *mfsk8_modem = 0;
modem *mfsk16_modem = 0;
modem *feld_modem = 0;
modem *feld_FMmodem = 0;
modem *feld_FM105modem = 0;
modem *feld_CMTmodem = 0;
modem *psk31_modem = 0;
modem *psk63_modem = 0;
modem *psk125_modem = 0;
modem *psk250_modem = 0;
modem *qpsk31_modem = 0;
modem *qpsk63_modem = 0;
modem *qpsk125_modem = 0;
modem *qpsk250_modem = 0;
modem *olivia_modem = 0;
modem *rtty_modem = 0;
modem *dominoex4_modem = 0;
modem *dominoex5_modem = 0;
modem *dominoex8_modem = 0;
modem *dominoex11_modem = 0;
modem *dominoex16_modem = 0;
modem *dominoex22_modem = 0;
modem *throb1_modem = 0;
modem *throb2_modem = 0;
modem *throb4_modem = 0;
modem *throbx1_modem = 0;
modem *throbx2_modem = 0;
modem *throbx4_modem = 0;
modem *wwv_modem = 0;
modem *anal_modem = 0;

trx_mode modem::get_mode()
{
	return mode;
}

modem::modem()
{
	twopi = 2.0 * M_PI;
	scptr = 0;
	freqlock = false;
	sigsearch = 0;
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	reverse = wfrev ^ !wfsb;
	afcon = true;
	squelchon = true;
}

void modem::init()
{
	afcon = progdefaults.afconoff;
	squelchon = progdefaults.sqlonoff;
	squelch = progdefaults.sldrSquelchValue;
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	reverse = wfrev ^ !wfsb;
	
	if (progdefaults.StartAtSweetSpot) {
		if (active_modem == cw_modem)
			set_freq(progdefaults.CWsweetspot);
		else if (active_modem == rtty_modem)
			set_freq(progdefaults.RTTYsweetspot);
		else
			set_freq(progdefaults.PSKsweetspot);
	} else if (progStatus.carrier != 0) {
			set_freq(progStatus.carrier);
			progStatus.carrier = 0;
	} else
		set_freq(wf->Carrier());
}

void modem::set_freq(double freq)
{
	frequency = freq;
	freqerr = 0.0;
	if (freqlock == false)
		tx_frequency = frequency;
	REQ(put_freq, frequency);
}

void modem::set_freqlock(bool on)
{
	if (on == false)
		tx_frequency = frequency;
	freqlock = on;
}


bool modem::freqlocked()
{
	return freqlock;
}

double modem::get_txfreq(void)
{
	return tx_frequency;
}

double modem::get_txfreq_woffset(void)
{
	return (tx_frequency - progdefaults.TxOffset);
}

int modem::get_freq()
{
	return (int)(frequency + 0.5);
}

double modem::get_bandwidth(void)
{
	return bandwidth;
}

void modem::set_bandwidth(double bw)
{
	bandwidth = bw;
	put_Bandwidth((int)bandwidth);
}

void modem::set_reverse(bool on)
{
	reverse = on ^ (!wf->USB());
}

void modem::set_metric(double m)
{
	metric = m;
}

double modem::get_metric(void)
{
	return metric;
}


bool modem::get_cwTrack()
{
	return cwTrack;
}

void modem::set_cwTrack(bool b)
{
	cwTrack = b;
}

bool modem::get_cwLock()
{
	return cwLock;
}

void modem::set_cwLock(bool b)
{
	cwLock = b;
}

double modem::get_cwRcvWPM()
{
	return cwRcvWPM;
}

double modem::get_cwXmtWPM()
{
	return cwXmtWPM;
}

void modem::set_cwXmtWPM(double wpm)
{
	cwXmtWPM = wpm;
}
	
int modem::get_samplerate(void)
{
	return samplerate;
}

void modem::set_samplerate(int smprate)
{
	samplerate = smprate;
}

mbuffer<double, 512 * 2, 2> _mdm_scdbl;

void modem::ModulateXmtr(double *buffer, int len) 
{
	scard->write_samples(buffer, len);

	if (!progdefaults.viewXmtSignal)
		return;
	for (int i = 0; i < len; i++) {
		_mdm_scdbl[scptr] = buffer[i] * 0.5;
		scptr++;
		if (scptr == 512) {
			REQ(&waterfall::sig_data, wf, _mdm_scdbl.c_array(), 512);
			scptr = 0;
			_mdm_scdbl.next(); // change buffers
		}
	}
}

void modem::ModulateStereo(double *left, double *right, int len)
{
	scard->write_stereo(left, right, len);

	if (!progdefaults.viewXmtSignal)
		return;
	for (int i = 0; i < len; i++) {
		_mdm_scdbl[scptr] = left[i] * 0.5;
		scptr++;
		if (scptr == 512) {
			REQ(&waterfall::sig_data, wf, _mdm_scdbl.c_array(), 512);
			scptr = 0;
			_mdm_scdbl.next(); // change buffers
		}
	}
}


void modem::videoText()
{
	if (trx_state == STATE_TUNE)
		return;
	if (progdefaults.sendtextid == true) {
		wfid_text(progdefaults.strTextid);
	} else if (progdefaults.macrotextid == true) {
		wfid_text(progdefaults.strTextid);
		progdefaults.macrotextid = false;
	}
	if (progdefaults.sendid == true) {
		wfid_text(mode_info[mode].sname);
	} else if (progdefaults.macroid == true) {
		wfid_text(mode_info[mode].sname);
		progdefaults.macroid = false;
	}
}

//=====================================================================
// transmit processing of waterfall video id
//=====================================================================
//#define progdefaults.videowidth			3
#define MAXCHARS			4
#define NUMCOLS				8
#define MAXBITS				7
#define NUMROWS				14
#define CHARSPACE			2
#define TONESPACING			8
#define IDSYMLEN			4096
#define RISETIME			20

struct mfntchr { char c; int byte[NUMROWS]; };
extern mfntchr idch[];

void modem::wfid_make_pulse()
{
	double risetime = (samplerate / 1000) * RISETIME;
	for (int i = 0; i < IDSYMLEN; i++)
		wfid_txpulse[i] = 1.0;
	for (int i = 0; i < risetime; i++)
		wfid_txpulse[i] = wfid_txpulse[IDSYMLEN - 1 - i] =
			0.5 * (1 - cos(M_PI * i / risetime));
}

void modem::wfid_make_tones()
{
	double f;
	f = frequency + TONESPACING * ( progdefaults.videowidth * ((NUMCOLS - 1)) / 2.0 + (progdefaults.videowidth - 1) * CHARSPACE);
	for (int i = 0; i < NUMCOLS * progdefaults.videowidth; i++) {
		wfid_w[i] = f * 2.0 * M_PI / samplerate;
		f -= TONESPACING;
		if ( (i > 0) && (i % NUMCOLS == 0) )
			f -= TONESPACING * CHARSPACE;
	}
}

void modem::wfid_send(long int symbol)
{
	int i, j;
	int sym;
	int msk = 0;
	for (i = 0; i < IDSYMLEN; i++) {
		wfid_outbuf[i] = 0.0;
		sym = symbol;
		for (j = 0; j < NUMCOLS * progdefaults.videowidth; j++) {
			if ((sym & 1) == 1)
				wfid_outbuf[i] += ((msk & 1) == 1 ? -1 : 1 ) * sin(wfid_w[j] * i)* wfid_txpulse[i];
			sym = sym >> 1;
			msk = msk >> 1;
		}
	}
	for (i = 0; i < IDSYMLEN; i++)
		wfid_outbuf[i] = wfid_outbuf[i] / (MAXBITS * progdefaults.videowidth);
		
	ModulateXmtr(wfid_outbuf, IDSYMLEN);
}

void modem::wfid_sendchar(char c)
{
// send rows from bottom to top so they appear to scroll down the waterfall correctly
	long int symbol;
	unsigned int n;
	if (c < ' ' || c > '~') return;
	n = c - ' ';
	for (int row = 0; row < NUMROWS; row++) {
		symbol = (idch[n].byte[NUMROWS - 1 -row]) >> (16 - NUMCOLS);
		wfid_send (symbol);
		if (stopflag)
			return;
	}
}

void modem::wfid_sendchars(string s)
{
	long int symbol;
	int  len = s.length();
	unsigned int  n[progdefaults.videowidth];
	int  c;
	while (len++ < progdefaults.videowidth) s.insert(0," ");

	for (int i = 0; i < progdefaults.videowidth; i++) {
		c = s[i];
		if (c > '~' || c < ' ') c = ' ';
		c -= ' ';
		n[i] = c;
	}
// send rows from bottom to top so they appear to scroll down the waterfall correctly
	for (int row = 0; row < NUMROWS; row++) {
		symbol = 0;
		for (int i = 0; i < progdefaults.videowidth; i++) {
			symbol |= (idch[n[i]].byte[NUMROWS - 1 -row] >> (16 - NUMCOLS));
			if (i != (progdefaults.videowidth - 1) )
				symbol = symbol << NUMCOLS;
		}
		wfid_send (symbol);
		if (stopflag)
			return;
	}
}

void modem::wfid_text(string s)
{
	int len = s.length();
	string video = "Video text: ";
	video += s;
	
	wfid_make_pulse();
	wfid_make_tones();
	
	put_status(video.c_str());

	if (progdefaults.videowidth == 1) {
		for (int i = len - 1; i >= 0; i--) {
			wfid_sendchar(s[i]);
		}
	} else {
		int numlines = 0;
		string tosend;
		while (numlines < len) numlines += progdefaults.videowidth;
		numlines -= progdefaults.videowidth;
		while (numlines >= 0) {
			tosend = s.substr(numlines, progdefaults.videowidth);
			wfid_sendchars(tosend);
			numlines -= progdefaults.videowidth;
			if (stopflag)
				break;
		}
	}
	put_status("");
}

double	modem::wfid_txpulse[IDSYMLEN];
double	modem::wfid_outbuf[IDSYMLEN];
double  modem::wfid_w[NUMCOLS * MAXCHARS];

mfntchr idch[] = {
{' ', { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, },
{'!', { 0x0000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0x0000, 0x0000, 0xC000, 0xC000, 0x0000, 0x0000, 0x0000 }, },
{'"', { 0x0000, 0xD800, 0xD800, 0xD800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, },
{'#', { 0x0000, 0x5000, 0x5000, 0xF800, 0xF800, 0x5000, 0x5000, 0xF800, 0xF800, 0x5000, 0x5000, 0x0000, 0x0000, 0x0000 }, },
{'$', { 0x0000, 0x2000, 0x2000, 0x7800, 0xF800, 0xA000, 0xF000, 0x7800, 0x2800, 0xF800, 0xF000, 0x2000, 0x2000, 0x0000 }, },
{'%', { 0x0000, 0x4000, 0xE400, 0xE400, 0x4C00, 0x1800, 0x3000, 0x6000, 0xC800, 0x9C00, 0x9C00, 0x8800, 0x0000, 0x0000 }, },
{'&', { 0x0000, 0x3000, 0x7800, 0x4800, 0x4800, 0x7000, 0xF400, 0x8C00, 0x8800, 0xFC00, 0x7400, 0x0000, 0x0000, 0x0000 }, },
{ 39, { 0x0000, 0x4000, 0x4000, 0xC000, 0x8000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, },
{'(', { 0x0000, 0x0000, 0x2000, 0x6000, 0xC000, 0x8000, 0x8000, 0x8000, 0x8000, 0xC000, 0x6000, 0x2000, 0x0000, 0x0000 }, },
{')', { 0x0000, 0x0000, 0x8000, 0xC000, 0x6000, 0x2000, 0x2000, 0x2000, 0x2000, 0x6000, 0xC000, 0x8000, 0x0000, 0x0000 }, },
{'*', { 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0xFE00, 0x7C00, 0x3800, 0x6C00, 0x4400, 0x0000, 0x0000, 0x0000, 0x0000 }, },
{'+', { 0x0000, 0x0000, 0x0000, 0x2000, 0x2000, 0x2000, 0xF800, 0xF800, 0x2000, 0x2000, 0x2000, 0x0000, 0x0000, 0x0000 }, },
{',', { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xC000, 0xC000, 0xC000, 0x4000, 0xC000, 0x8000 }, },
{'-', { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xF800, 0xF800, 0x0000, 0x0000 }, },
{'.', { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xC000, 0xC000, 0xC000, 0x0000, 0x0000 }, },
{'/', { 0x0000, 0x0800, 0x0800, 0x1800, 0x1000, 0x3000, 0x2000, 0x6000, 0x4000, 0xC000, 0x8000, 0x8000, 0x0000, 0x0000 }, },
{'0', { 0x0000, 0x0000, 0x7800, 0xFC00, 0x8C00, 0x9C00, 0xB400, 0xE400, 0xC400, 0x8400, 0xFC00, 0x7800, 0x0000, 0x0000 }, },
{'1', { 0x0000, 0x0000, 0x2000, 0x6000, 0xE000, 0xA000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x0000, 0x0000 }, },
{'2', { 0x0000, 0x0000, 0x7800, 0xFC00, 0x8400, 0x0C00, 0x1800, 0x3000, 0x6000, 0xC000, 0xFC00, 0xFC00, 0x0000, 0x0000 }, },
{'3', { 0x0000, 0x0000, 0xFC00, 0xFC00, 0x0400, 0x0C00, 0x1800, 0x1C00, 0x0400, 0x8400, 0xFC00, 0x7800, 0x0000, 0x0000 }, },
{'4', { 0x0000, 0x0000, 0x3800, 0x7800, 0x4800, 0xC800, 0x8800, 0xFC00, 0xFC00, 0x0800, 0x0800, 0x0800, 0x0000, 0x0000 }, },
{'5', { 0x0000, 0x0000, 0xFC00, 0xFC00, 0x8000, 0x8000, 0xF800, 0xFC00, 0x0400, 0x0400, 0xFC00, 0xF800, 0x0000, 0x0000 }, },
{'6', { 0x0000, 0x0000, 0x7800, 0xF800, 0x8000, 0x8000, 0xF800, 0xFC00, 0x8400, 0x8400, 0xFC00, 0x7800, 0x0000, 0x0000 }, },
{'7', { 0x0000, 0x0000, 0xFC00, 0xFC00, 0x0400, 0x0400, 0x0C00, 0x1800, 0x3000, 0x2000, 0x2000, 0x2000, 0x0000, 0x0000 }, },
{'8', { 0x0000, 0x0000, 0x7800, 0xFC00, 0x8400, 0x8400, 0x7800, 0xFC00, 0x8400, 0x8400, 0xFC00, 0x7800, 0x0000, 0x0000 }, },
{'9', { 0x0000, 0x0000, 0x7800, 0xFC00, 0x8400, 0x8400, 0xFC00, 0x7C00, 0x0400, 0x0400, 0x7C00, 0x7800, 0x0000, 0x0000 }, },
{':', { 0x0000, 0xC000, 0xC000, 0xC000, 0x0000, 0x0000, 0x0000, 0xC000, 0xC000, 0xC000, 0x0000, 0x0000, 0x0000, 0x0000 }, },
{';', { 0x0000, 0x6000, 0x6000, 0x6000, 0x0000, 0x0000, 0x6000, 0x6000, 0x2000, 0x2000, 0xE000, 0xC000, 0x0000, 0x0000 }, },
{'<', { 0x0000, 0x0000, 0x0800, 0x1800, 0x3000, 0x6000, 0xC000, 0xC000, 0x6000, 0x3000, 0x1800, 0x0800, 0x0000, 0x0000 }, },
{'=', { 0x0000, 0x0000, 0x0000, 0x0000, 0xF800, 0xF800, 0x0000, 0x0000, 0xF800, 0xF800, 0x0000, 0x0000, 0x0000, 0x0000 }, },
{'>', { 0x0000, 0x0000, 0x8000, 0xC000, 0x6000, 0x3000, 0x1800, 0x1800, 0x3000, 0x6000, 0xC000, 0x8000, 0x0000, 0x0000 }, },
{'?', { 0x0000, 0x0000, 0x7000, 0xF800, 0x8800, 0x0800, 0x1800, 0x3000, 0x2000, 0x0000, 0x2000, 0x2000, 0x0000, 0x0000 }, },
{'@', { 0x0000, 0x0000, 0x7C00, 0xFE00, 0x8200, 0x8200, 0xB200, 0xBE00, 0xBC00, 0x8000, 0xFC00, 0x7C00, 0x0000, 0x0000 }, },
{'A', { 0x0000, 0x0000, 0x3000, 0x7800, 0xCC00, 0x8400, 0x8400, 0xFC00, 0xFC00, 0x8400, 0x8400, 0x8400, 0x0000, 0x0000 }, },
{'B', { 0x0000, 0x0000, 0xF800, 0xFC00, 0x8400, 0x8400, 0xF800, 0xF800, 0x8400, 0x8400, 0xFC00, 0xF800, 0x0000, 0x0000 }, },
{'C', { 0x0000, 0x0000, 0x3800, 0x7C00, 0xC400, 0x8000, 0x8000, 0x8000, 0x8000, 0xC400, 0x7C00, 0x3800, 0x0000, 0x0000 }, },
{'D', { 0x0000, 0x0000, 0xF000, 0xF800, 0x8C00, 0x8400, 0x8400, 0x8400, 0x8400, 0x8C00, 0xF800, 0xF000, 0x0000, 0x0000 }, },
{'E', { 0x0000, 0x0000, 0xFC00, 0xFC00, 0x8000, 0x8000, 0xF000, 0xF000, 0x8000, 0x8000, 0xFC00, 0xFC00, 0x0000, 0x0000 }, },
{'F', { 0x0000, 0x0000, 0xFC00, 0xFC00, 0x8000, 0x8000, 0xF000, 0xF000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0000, 0x0000 }, },
{'G', { 0x0000, 0x0000, 0x3C00, 0x7C00, 0xC000, 0x8000, 0x8C00, 0x8C00, 0x8400, 0xC400, 0x7C00, 0x3800, 0x0000, 0x0000 }, },
{'H', { 0x0000, 0x0000, 0x8400, 0x8400, 0x8400, 0x8400, 0xFC00, 0xFC00, 0x8400, 0x8400, 0x8400, 0x8400, 0x0000, 0x0000 }, },
{'I', { 0x0000, 0x0000, 0xF800, 0xF800, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0xF800, 0xF800, 0x0000, 0x0000 }, },
{'J', { 0x0000, 0x0000, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x8400, 0x8400, 0xFC00, 0x7800, 0x0000, 0x0000 }, },
{'K', { 0x0000, 0x0000, 0x8400, 0x8400, 0x8C00, 0x9800, 0xF000, 0xF000, 0x9800, 0x8C00, 0x8400, 0x8400, 0x0000, 0x0000 }, },
{'L', { 0x0000, 0x0000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0xFC00, 0xFC00, 0x0000, 0x0000 }, },
{'M', { 0x0000, 0x0000, 0x8200, 0xC600, 0xEE00, 0xBA00, 0x9200, 0x8200, 0x8200, 0x8200, 0x8200, 0x8200, 0x0000, 0x0000 }, },
{'N', { 0x0000, 0x0000, 0x8400, 0xC400, 0xE400, 0xB400, 0x9C00, 0x8C00, 0x8400, 0x8400, 0x8400, 0x8400, 0x0000, 0x0000 }, },
{'O', { 0x0000, 0x0000, 0x3000, 0x7800, 0xCC00, 0x8400, 0x8400, 0x8400, 0x8400, 0xCC00, 0x7800, 0x3000, 0x0000, 0x0000 }, },
{'P', { 0x0000, 0x0000, 0xF800, 0xFC00, 0x8400, 0x8400, 0xFC00, 0xF800, 0x8000, 0x8000, 0x8000, 0x8000, 0x0000, 0x0000 }, },
{'Q', { 0x0000, 0x0000, 0x7800, 0xFC00, 0x8400, 0x8400, 0x8400, 0x8400, 0x9400, 0x9400, 0xFC00, 0x7800, 0x0800, 0x0800 }, },
{'R', { 0x0000, 0x0000, 0xF800, 0xFC00, 0x8400, 0x8400, 0xFC00, 0xF800, 0x8800, 0x8C00, 0x8400, 0x8400, 0x0000, 0x0000 }, },
{'S', { 0x0000, 0x0000, 0x7800, 0xFC00, 0x8400, 0x8000, 0xF800, 0x7C00, 0x0400, 0x8400, 0xFC00, 0x7800, 0x0000, 0x0000 }, },
{'T', { 0x0000, 0x0000, 0xF800, 0xF800, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x0000, 0x0000 }, },
{'U', { 0x0000, 0x0000, 0x8400, 0x8400, 0x8400, 0x8400, 0x8400, 0x8400, 0x8400, 0x8400, 0xFC00, 0x7800, 0x0000, 0x0000 }, },
{'V', { 0x0000, 0x0000, 0x8200, 0x8200, 0x8200, 0xC600, 0x4400, 0x6C00, 0x2800, 0x3800, 0x1000, 0x1000, 0x0000, 0x0000 }, },
{'W', { 0x0000, 0x0000, 0x8200, 0x8200, 0x8200, 0x8200, 0x8200, 0x9200, 0x9200, 0x9200, 0xFE00, 0x6C00, 0x0000, 0x0000 }, },
{'X', { 0x0000, 0x0000, 0x8200, 0x8200, 0xC600, 0x6C00, 0x3800, 0x3800, 0x6C00, 0xC600, 0x8200, 0x8200, 0x0000, 0x0000 }, },
{'Y', { 0x0000, 0x0000, 0x8200, 0x8200, 0xC600, 0x6C00, 0x3800, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x0000, 0x0000 }, },
{'Z', { 0x0000, 0x0000, 0xFC00, 0xFC00, 0x0C00, 0x1800, 0x3000, 0x6000, 0xC000, 0x8000, 0xFC00, 0xFC00, 0x0000, 0x0000 }, },
{'[', { 0x0000, 0x0000, 0xE000, 0xE000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0xE000, 0xE000, 0x0000, 0x0000 }, },
{'\\', { 0x0000, 0x8000, 0x8000, 0xC000, 0x4000, 0x6000, 0x2000, 0x3000, 0x1000, 0x1800, 0x0800, 0x0800, 0x0000, 0x0000 }, },
{']', { 0x0000, 0x0000, 0xE000, 0xE000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0xE000, 0xE000, 0x0000, 0x0000 }, },
{'^', { 0x0000, 0x2000, 0x2000, 0x7000, 0x5000, 0xD800, 0x8800, 0x8800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, },
{'_', { 0x0000, 0xF800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xF800, 0x0000, 0x0000 }, },
{'`', { 0x0000, 0xC000, 0xC000, 0xC000, 0xC000, 0x6000, 0x6000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, },
{'a', { 0x0000, 0x0000, 0x0000, 0x0000, 0x7800, 0x7C00, 0x0400, 0x7C00, 0xFC00, 0x8400, 0xFC00, 0x7C00, 0x0000, 0x0000 }, },
{'b', { 0x0000, 0x0000, 0x8000, 0x8000, 0xB800, 0xFC00, 0xC400, 0x8400, 0x8400, 0x8400, 0xFC00, 0xF800, 0x0000, 0x0000 }, },
{'c', { 0x0000, 0x0000, 0x0000, 0x0000, 0x7800, 0xF800, 0x8000, 0x8000, 0x8000, 0x8000, 0xF800, 0x7800, 0x0000, 0x0000 }, },
{'d', { 0x0000, 0x0000, 0x0400, 0x0400, 0x7400, 0xFC00, 0x8C00, 0x8400, 0x8400, 0x8400, 0xFC00, 0x7C00, 0x0000, 0x0000 }, },
{'e', { 0x0000, 0x0000, 0x0000, 0x0000, 0x7800, 0xFC00, 0x8400, 0xFC00, 0xFC00, 0x8000, 0xF800, 0x7800, 0x0000, 0x0000 }, },
{'f', { 0x0000, 0x0000, 0x3C00, 0x7C00, 0x4000, 0x4000, 0xF800, 0xF800, 0x4000, 0x4000, 0x4000, 0x4000, 0x0000, 0x0000 }, },
{'g', { 0x0000, 0x0000, 0x0000, 0x7C00, 0xFC00, 0x8400, 0x8400, 0x8C00, 0xFC00, 0x7400, 0x0400, 0x7C00, 0x7800, 0x0000 }, },
{'h', { 0x0000, 0x0000, 0x8000, 0x8000, 0xB800, 0xFC00, 0xC400, 0x8400, 0x8400, 0x8400, 0x8400, 0x8400, 0x0000, 0x0000 }, },
{'i', { 0x0000, 0x2000, 0x2000, 0x0000, 0xE000, 0xE000, 0x2000, 0x2000, 0x2000, 0x2000, 0xF800, 0xF800, 0x0000, 0x0000 }, },
{'j', { 0x0000, 0x0800, 0x0800, 0x0000, 0x3800, 0x3800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x8800, 0xF800, 0x7000 }, },
{'k', { 0x0000, 0x0000, 0x8000, 0x8800, 0x9800, 0xB000, 0xE000, 0xE000, 0xB000, 0x9800, 0x8800, 0x8800, 0x0000, 0x0000 }, },
{'l', { 0x0000, 0x0000, 0xE000, 0xE000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0xF800, 0xF800, 0x0000, 0x0000 }, },
{'m', { 0x0000, 0x0000, 0x0000, 0x0000, 0xEC00, 0xFE00, 0x9200, 0x9200, 0x8200, 0x8200, 0x8200, 0x8200, 0x0000, 0x0000 }, },
{'n', { 0x0000, 0x0000, 0x0000, 0x0000, 0xB800, 0xFC00, 0xC400, 0x8400, 0x8400, 0x8400, 0x8400, 0x8400, 0x0000, 0x0000 }, },
{'o', { 0x0000, 0x0000, 0x0000, 0x0000, 0x7800, 0xFC00, 0x8400, 0x8400, 0x8400, 0x8400, 0xFC00, 0x7800, 0x0000, 0x0000 }, },
{'p', { 0x0000, 0x0000, 0x0000, 0x0000, 0xF800, 0xFC00, 0x8400, 0x8400, 0xC400, 0xFC00, 0xB800, 0x8000, 0x8000, 0x8000 }, },
{'q', { 0x0000, 0x0000, 0x0000, 0x0000, 0x7C00, 0xFC00, 0x8400, 0x8400, 0x8C00, 0xFC00, 0x7400, 0x0400, 0x0400, 0x0400 }, },
{'r', { 0x0000, 0x0000, 0x0000, 0x0000, 0xB800, 0xFC00, 0xC400, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0000, 0x0000 }, },
{'s', { 0x0000, 0x0000, 0x0000, 0x0000, 0x7C00, 0xFC00, 0x8000, 0xF800, 0x7C00, 0x0400, 0xFC00, 0xF800, 0x0000, 0x0000 }, },
{'t', { 0x0000, 0x0000, 0x4000, 0x4000, 0xF000, 0xF000, 0x4000, 0x4000, 0x4000, 0x4000, 0x7800, 0x3800, 0x0000, 0x0000 }, },
{'u', { 0x0000, 0x0000, 0x0000, 0x0000, 0x8400, 0x8400, 0x8400, 0x8400, 0x8400, 0x8C00, 0xFC00, 0x7400, 0x0000, 0x0000 }, },
{'v', { 0x0000, 0x0000, 0x0000, 0x0000, 0x8200, 0x8200, 0x8200, 0x8200, 0xC600, 0x6C00, 0x3800, 0x1000, 0x0000, 0x0000 }, },
{'w', { 0x0000, 0x0000, 0x0000, 0x0000, 0x8200, 0x8200, 0x8200, 0x9200, 0x9200, 0x9200, 0xFE00, 0x6C00, 0x0000, 0x0000 }, },
{'x', { 0x0000, 0x0000, 0x0000, 0x0000, 0x8200, 0xC600, 0x6C00, 0x3800, 0x3800, 0x6C00, 0xC600, 0x8200, 0x0000, 0x0000 }, },
{'y', { 0x0000, 0x0000, 0x0000, 0x0000, 0x8400, 0x8400, 0x8400, 0x8400, 0x8C00, 0xFC00, 0x7400, 0x0400, 0x7C00, 0x7800 }, },
{'z', { 0x0000, 0x0000, 0x0000, 0x0000, 0xFC00, 0xFC00, 0x1800, 0x3000, 0x6000, 0xC000, 0xFC00, 0xFC00, 0x0000, 0x0000 }, },
{'{', { 0x0000, 0x2000, 0x6000, 0x4000, 0x4000, 0x4000, 0xC000, 0xC000, 0x4000, 0x4000, 0x4000, 0x6000, 0x2000, 0x0000 }, },
{'|', { 0x0000, 0x8000, 0x8000, 0xC000, 0x4000, 0x6000, 0x2000, 0x3000, 0x1000, 0x1800, 0x0800, 0x0800, 0x0000, 0x0000 }, },
{'}', { 0x0000, 0x8000, 0xC000, 0x4000, 0x4000, 0x4000, 0x6000, 0x6000, 0x4000, 0x4000, 0x4000, 0xC000, 0x8000, 0x0000 }, },
{'~', { 0x0000, 0x9800, 0xFC00, 0x6400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, }
};

// CW ID transmit routines

//===========================================================================
// cw transmit routines to send a post amble message
// Define the amplitude envelop for key down events      
// this is 1/2 cycle of a raised cosine                                    
//===========================================================================

void modem::cwid_makeshape()
{
	for (int i = 0; i < 128; i++) cwid_keyshape[i] = 1.0;
	for (int i = 0; i < RT; i++)
		cwid_keyshape[i] = 0.5 * (1.0 - cos (M_PI * i / RT));
}

double modem::cwid_nco(double freq)
{
	cwid_phaseacc += 2.0 * M_PI * freq / samplerate;

	if (cwid_phaseacc > M_PI)
		cwid_phaseacc -= 2.0 * M_PI;

	return sin(cwid_phaseacc);
}

//=====================================================================
// cwid_send_symbol()
// Sends a part of a morse character (one dot duration) of either
// sound at the correct freq or silence. Rise and fall time is controlled
// with a raised cosine shape.
//
// Left channel contains the shaped A2 CW waveform
//=======================================================================

void modem::cwid_send_symbol(int bits)
{
	double freq;
	int i,
		keydown,
		keyup,
		sample = 0, 
		currsym = bits & 1;
	
	freq = tx_frequency - progdefaults.TxOffset;

    if ((currsym == 1) && (cwid_lastsym == 0))
    	cwid_phaseacc = 0.0;

	keydown = cwid_symbollen - RT;
	keyup = cwid_symbollen - RT;
	
	if (currsym == 1) {
		for (i = 0; i < RT; i++, sample++) {
			if (cwid_lastsym == 0)
				outbuf[sample] = cwid_nco(freq) * cwid_keyshape[i];
			else
				outbuf[sample] = cwid_nco(freq);
		}
		for (i = 0; i < keydown; i++, sample++) {
			outbuf[sample] = cwid_nco(freq);
		}
	}
	else {
		for (i = RT - 1; i >= 0; i--, sample++) {
			if (cwid_lastsym == 1) {
				outbuf[sample] = cwid_nco(freq) * cwid_keyshape[i];
			} else {
				outbuf[sample] = 0.0;
			}
		}
		for (i = 0; i < keyup; i++, sample++) {
			outbuf[sample] = 0.0;
		}
	}

	ModulateXmtr(outbuf, cwid_symbollen);
	
	cwid_lastsym = currsym;
}

//=====================================================================
// send_ch()
// sends a morse character and the space afterwards
//=======================================================================

void modem::cwid_send_ch(int ch)
{
	int code;

// handle word space separately (7 dots spacing) 
// last char already had 2 elements of inter-character spacing 

	if ((ch == ' ') || (ch == '\n')) {
		cwid_send_symbol(0);
		cwid_send_symbol(0);
		cwid_send_symbol(0);
		cwid_send_symbol(0);
		cwid_send_symbol(0);
		put_echo_char(ch);
		return;
	}

// convert character code to a morse representation 
	if ((ch < 256) && (ch >= 0)) {
		code = tx_lookup(ch); //cw_tx_lookup(ch);
	} else {
		code = 0x04; 	// two extra dot spaces
	}

// loop sending out binary bits of cw character 
	while (code > 1) {
		cwid_send_symbol(code);// & 1);
		code = code >> 1;
	}

}

void modem::cwid_sendtext (string s)
{
	cwid_symbollen = (int)(1.2 * samplerate / progdefaults.CWIDwpm);
	RT = (int) (samplerate * 6 / 1000.0); // 6 msec risetime for CW pulse
	cwid_makeshape();
	cwid_lastsym = 0;
	for (unsigned int i = 0; i < s.length(); i++) {
		cwid_send_ch(s[i]);
	}
}

void modem::cwid()
{
	if (progdefaults.CWid == true || progdefaults.macroCWid == true) {
		string tosend = " DE ";
		tosend += progdefaults.myCall;
		cwid_sendtext(tosend);
		progdefaults.macroCWid = false;
	}
}

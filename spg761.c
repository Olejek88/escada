#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "utils.h"
#include "regex_utils.h"
#include "device.h"
#include "errors.h"
#include "eval.h"

#include "spg761.h"
#include <wchar.h>
#include <locale.h>

/* */

struct spg761_ctx {

	char *send_query;
	int  send_query_sz;

	regex_t re;

	char crc_flag;

	uint16_t crc;
};

#define get_spec(dev)	((struct spg761_ctx *)((dev)->spec))

/* */
static int  spg761_init(struct device *);
static void spg761_free(struct device *);
static int  spg761_get(struct device *, int, char **);
static int  spg761_set(struct device *, int, char *, char **);

static int  spg761_parse_msg(struct device *, char *, int);
static int  spg761_parse_crc_msg(struct device *, char *, int);
static int  spg761_check_crc(struct device *, char *, int);
static int  spg761_send_msg(struct device *);

static int  spg761_h_archiv(struct device *, int, const char *, const char *,
				struct archive **);
static int  spg761_m_archiv(struct device *, int, const char *, const char *,
				struct archive **);
static int  spg761_d_archiv(struct device *, int, const char *, const char *,
				struct archive **);
static int  spg761_events(struct device *, const char *, const char *,
				struct events **);
static int  generate_sequence (struct device *dev, uint8_t type, uint8_t func, const char* padr, uint8_t nchan, uint8_t npipe, int no, struct tm* from, struct tm* to, char* sequence);
static int  analyse_sequence (char* dats, uint len, char* answer, uint8_t analyse, uint8_t id, AnsLog *alog);

static int  conv_to_utf8 (unsigned char* dat, wchar_t* utf);

void * spg761DeviceThread (void * devr)
{
 int devdk=*((int*) devr);
 dbase.sqlconn("","root","");

 LoadSPG761Config();

 if (dev_num[TYPE_INPUTSPG761]>0)
    {
	for (UINT d=0;d<device_num;d++)
	if (dev[d].type==TYPE_INPUTSPG761)
    		 rs=OpenTTY (dev[d].port, dev[d].speed);
    }
 else return (0);
}


// crc code for n bytes, from *ptr sources. use polinom: (X ^ 16)+(X ^ 12)+(X ^ 5)+1 with bit mask 0x1021.
static uint16_t
calc_bcc(uint8_t *ptr, int n)
{
    uint16_t crc=0,j;

    while (n-- > 0)
	{
	 crc = crc ^ (int) *ptr++ << 8;
	 for (j=0;j<8;j++)
		{
		 if(crc&0x8000) crc = (crc << 1) ^ 0x1021;
		 else crc <<= 1;
		}
	}
    return crc;
}

static int
set_ctx_regex(struct spg761_ctx *ctx, const char *restr)
{
	regex_t re;
	int rv;

	rv = regcomp(&re, restr, REG_EXTENDED);
	if (0 == rv) {
		ctx->re = re;
	} else {
		set_regex_error(rv, &re);
		rv = -1;
	}
	return rv;
}

static void
ctx_zero(struct spg761_ctx *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
}

static int
spg761_get_ident(struct device *dev)
{
	static char ident_query[100];
	struct spg761_ctx *ctx;
	char a98_id[100];
	char spg_id[100];
	char spg_num[100];

	int ret;
	struct 	tm *time_from,*time_to;
	AnsLog	alog;

	time_t tim;
	tim=time(&tim);			// default values
	time_from=localtime(&tim); 	// get current system time
	time_to=localtime(&tim); 	// get current system time

	ctx = get_spec(dev);
	ret = -1;
	ctx->crc_flag = 0;

//	buf_init (&dev->buf);
	if (-1 == generate_sequence (dev, 0, 0x88, "99", 0, 0, 1, time_from, time_to, ident_query)) goto out;
	ctx->send_query = ident_query;
	ctx->send_query_sz = strlen(ident_query);
	if (-1 == dev_query(dev)) goto free_regex;
	if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, a98_id, ANALYSE, 0, &alog)) goto out;
	buf_free(&dev->buf);

	if (-1 == generate_sequence (dev, 0, FNC_readindex, "99", 0, 0, 1, time_from, time_to, ident_query)) goto out;
	ctx->send_query = ident_query;
	ctx->send_query_sz = strlen(ident_query);
	if (-1 == dev_query(dev)) goto free_regex;
	if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, spg_id, ANALYSE, 0, &alog)) goto out;
	buf_free(&dev->buf);

	if (-1 == generate_sequence (dev, 0, FNC_readindex, "99", 1, 0, 1, time_from, time_to, ident_query)) goto out;
	ctx->send_query = ident_query;
	ctx->send_query_sz = strlen(ident_query);
	if (-1 == dev_query(dev)) goto free_regex;
	if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, spg_num, ANALYSE, 0, &alog)) goto out;

	devlog(dev,
		"\tA98 identificator = %s\n"
		"\tSPG ident[099]=%s | code=%s\n", a98_id, spg_id, spg_num);

	ret = 0;

free_regex:
	regfree(&ctx->re);
out:
	ctx_zero(ctx);
	buf_free(&dev->buf);
	return ret;
}

static int
spg761_get_string_param(struct device *dev, const char *addr, char **save, uint8_t type, uint16_t padr)
{
	struct 	spg761_ctx *ctx;
	static char ident_query[100],	answer[1024], date[50];
	void 	*fnsave;
	char	*value_string;
	int 	ret;
	struct 	tm *time_from,*time_to;
	AnsLog	alog;
	time_t tim;
	tim=time(&tim);			// default values
	time_from=localtime(&tim); 	// get current system time
	time_to=localtime(&tim); 	// get current system time

	ctx = get_spec(dev);
	ret = -1;

	/* save previous handler */
	fnsave = dev->opers->parse_msg;
	dev->opers->parse_msg = spg761_parse_crc_msg;

	if (type==TYPE_CURRENTS) {
		if (-1 == generate_sequence (dev, 0, FNC_read, addr, 0, 1, 0, time_from, time_to, ident_query)) goto out;
		ctx->send_query = ident_query;
		ctx->send_query_sz = strlen(ident_query);
		if (-1 == dev_query(dev)) goto out;
		if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, answer, ANALYSE, 0, &alog)) goto out;
		buf_free(&dev->buf);
		if (!dev->quiet)
		    devlog(dev, "RECV: %s [%s]\n", alog.data[0][0],alog.time[0][0]);
		value_string=malloc (100);
		snprintf (value_string,90,"%s",alog.data[0][0]);
		*save=value_string;
		}
	if (type==TYPE_CONST) {
		if (padr>=1000) {
		    padr-=1000;
		    if (-1 == generate_sequence (dev, 0, FNC_readindex, addr, padr, 1, 1, time_from, time_to, ident_query)) goto out; // was 0
		    }
		else
		    if (-1 == generate_sequence (dev, 0, FNC_read, addr, 0, 0, 1, time_from, time_to, ident_query)) goto out; // was 0
		ctx->send_query = ident_query;
		ctx->send_query_sz = strlen(ident_query);
		if (-1 == dev_query(dev)) goto out;
		if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, answer, ANALYSE, 0, &alog)) goto out;
		buf_free(&dev->buf);
		if (!dev->quiet)
		    devlog(dev, "RECV: %s [%s]\n", alog.data[0][0],alog.time[0][0]);


		if (atoi(addr)==60 || atoi(addr)==20) {
			snprintf (date,100,"%s",alog.data[0][0]);
			if (atoi(addr)==60) if (-1 == generate_sequence (dev, 0, FNC_read, "61", 0, 0, 0, time_from, time_to, ident_query)) goto out;
			if (atoi(addr)==20) if (-1 == generate_sequence (dev, 0, FNC_read, "21", 0, 0, 0, time_from, time_to, ident_query)) goto out;
			ctx->send_query = ident_query;
			ctx->send_query_sz = strlen(ident_query);
			if (-1 == dev_query(dev)) goto out;
			if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, answer, ANALYSE, 0, &alog)) goto out;
			buf_free(&dev->buf);
			if (!dev->quiet)
			    devlog(dev, "RECV: %s [%s]\n", alog.data[0][0],alog.time[0][0]);
			value_string=malloc (100);
			snprintf (value_string,90,"%s %s",date,alog.data[0][0]);
			*save=value_string;
		}
		else	{
			value_string=malloc (100);
			snprintf (value_string,90,"%s",alog.data[0][0]);
			*save=value_string;
			}
		    printf ("%s\n",value_string);
		}

out:
	regfree(&ctx->re);
//	free(ctx->send_query);
	/* restore handler */
	dev->opers->parse_msg = fnsave;

	buf_free(&dev->buf);
	ctx_zero(ctx);
	return ret;
}


static int
spg761_set_string_param(struct device *dev, const char *addr, char *load, uint8_t type, uint16_t padr)
{
	struct spg761_ctx *ctx;
	static char ident_query[100],	answer[1024], date[50], times[50];
	void *fnsave;
	int ret;
	struct 	tm *time_from,*time_to;
	AnsLog	alog;
	time_t tim;
	tim=time(&tim);			// default values
	time_from=localtime(&tim); 	// get current system time
	time_to=localtime(&tim); 	// get current system time

	ctx = get_spec(dev);
	ret = -1;

	/* save previous handler */
	fnsave = dev->opers->parse_msg;
	dev->opers->parse_msg = spg761_parse_crc_msg;

	if (type==TYPE_CONST) {
		if (padr==520 || padr==560)	{
			snprintf (date,8,"%s",load);
			strncpy (ident_query,date,sizeof(ident_query));
			strncpy (times,load+9,8);
		    }
		else	strncpy (ident_query,load,sizeof(ident_query));
		if (padr>=1000) {
		    if (-1 == generate_sequence (dev, 0, FNC_writeindex, addr, padr-1000, 1, 1, time_from, time_to, ident_query)) goto out;
		    }
		else	{
			    if (-1 == generate_sequence (dev, 0, FNC_write, addr, padr, 0, 1, time_from, time_to, ident_query)) goto out;
			    }
		ctx->send_query = ident_query;
		ctx->send_query_sz = strlen(ident_query);
		if (-1 == dev_query(dev)) goto out;
		if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, answer, ANALYSE, 0, &alog)) goto out;
		buf_free(&dev->buf);
		if (!dev->quiet)
		    devlog(dev, "RECV:[%s]\n", alog.data[0][0]);

		wchar_t ans[100];
		conv_to_utf8 ((unsigned char *)alog.data[0][0], ans);
		printf ("[%s][%ls]\n",alog.time[0][0],ans);

		if (padr<1000 && padr>500) {
			strncpy (ident_query,times,sizeof(ident_query));
			if (padr==520)
			    if (-1 == generate_sequence (dev, 0, FNC_write, "21", 0, 0, 0, time_from, time_to, ident_query)) goto out;
			if (padr==560)
			    if (-1 == generate_sequence (dev, 0, FNC_write, "61", 0, 0, 0, time_from, time_to, ident_query)) goto out;

			ctx->send_query = ident_query;
			ctx->send_query_sz = strlen(ident_query);
			if (-1 == dev_query(dev)) goto out;
			if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, answer, ANALYSE, 0, &alog)) goto out;
			buf_free(&dev->buf);
			if (!dev->quiet)
			    devlog(dev, "RECV: %s [%s]\n", alog.data[0][0],alog.time[0][0]);
		    }
		}

out:
	regfree(&ctx->re);
//	free(ctx->send_query);
	/* restore handler */
	dev->opers->parse_msg = fnsave;

	buf_free(&dev->buf);
	ctx_zero(ctx);
	return ret;
}

/*
 * Interface
 */
static struct vk_operations spg761_opers = {
	.init	= spg761_init,
	.free	= spg761_free,
	.get	= spg761_get,
	.set	= spg761_set,

	.send_msg  = spg761_send_msg,
	.parse_msg = spg761_parse_msg,
	.check_crc = spg761_check_crc,

	.h_archiv = spg761_h_archiv,
	.m_archiv = spg761_m_archiv,
	.d_archiv = spg761_d_archiv,

	.events = spg761_events
};

void
spg761_get_operations(struct vk_operations **p_opers)
{
	*p_opers = &spg761_opers;
}

static int
spg761_init(struct device *dev)
{
	struct spg761_ctx *spec;
	int ret=-1;

	spec = calloc(1, sizeof(*spec));

	if (!spec) {
		set_system_error("calloc");
		ret = -1;
	} else {
		dev->spec = spec;
		ret =   (-1 == spg761_get_ident(dev));
		if (!ret) {
			ret = 0;
		} else {
			free(spec);
			dev->spec = NULL;
			ret = -1;
		}
	}
	return ret;
}

static void
spg761_free(struct device *dev)
{
	struct spg761_ctx *spec = get_spec(dev);

	free(spec);
	dev->spec = NULL;
}

static int
spg761_get_paramaddr (int no, int type)
{
    int i,addr=-1;
    switch (type)
	{
	 case TYPE_CURRENTS: for (i=0; i<ARRAY_SIZE(archive_currents761); ++i)
				 if (no == archive_currents761[i].no) addr=archive_currents761[i].adr;
				 break;
	 case TYPE_HOURS: for (i=0; i<ARRAY_SIZE(archive_hour761); ++i)
				 if (no == archive_hour761[i].no) addr=archive_hour761[i].adr;
				 break;
	 case TYPE_DAYS: for (i=0; i<ARRAY_SIZE(archive_day761); ++i)
				 if (no == archive_day761[i].no) addr=archive_day761[i].adr;
				 break;
	 case TYPE_MONTH: for (i=0; i<ARRAY_SIZE(archive_month761); ++i)
				 if (no == archive_month761[i].no) addr=archive_month761[i].adr;
				 break;
	 case TYPE_CONST: for (i=0; i<ARRAY_SIZE(archive_const761); ++i)
				 if (no == archive_const761[i].no) addr=archive_const761[i].adr;
				 break;
	 default: addr=-1;
	}
    return addr;
}

static int
spg761_get_paramid (int no, int type)
{
    int i,id=-1;
    if (!type)
    switch (type)
	{
	 case TYPE_CURRENTS: for (i=0; i<ARRAY_SIZE(archive_currents761); ++i)
				 if (no == archive_currents761[i].no) id=archive_currents761[i].id;
				 break;
	 case TYPE_HOURS: for (i=0; i<ARRAY_SIZE(archive_hour761); ++i)
				 if (no == archive_hour761[i].no) id=archive_hour761[i].id;
				 break;
	 case TYPE_DAYS: for (i=0; i<ARRAY_SIZE(archive_day761); ++i)
				 if (no == archive_day761[i].no) id=archive_day761[i].id;
				 break;
	 case TYPE_MONTH: for (i=0; i<ARRAY_SIZE(archive_month761); ++i)
				 if (no == archive_month761[i].no) id=archive_month761[i].id;
				 break;
	 case TYPE_CONST: for (i=0; i<ARRAY_SIZE(archive_const761); ++i)
				 if (no == archive_const761[i].no) id=archive_const761[i].id;
				 break;

	 default: id=-1;
	}
    return id;
}

static int
spg761_get(struct device *dev, int param, char **ret)
{
	int i=0, rv=0;
	char	addr[10];
	i = spg761_get_paramaddr(param,TYPE_CURRENTS);
	if (i != -1) {
		sprintf (addr,"%d",i);
		for (i=0; i<ARRAY_SIZE(archive_currents761); i++)
		if (archive_currents761[i].no==param)	{
    		    rv = spg761_get_string_param(dev, addr, ret, TYPE_CURRENTS, archive_currents761[i].type);
		    return 0;
		    }
	}
	i = spg761_get_paramaddr(param,TYPE_CONST);
	if (i != -1) {
		sprintf (addr,"%d",i);
		for (i=0; i<ARRAY_SIZE(archive_const761); i++)
		if (archive_const761[i].no==param)	{
		    rv = spg761_get_string_param(dev, addr, ret, TYPE_CONST, archive_const761[i].type);
		    return 0;
		    }
    	}
	return -1;
}

static int
spg761_set(struct device *dev, int param, char* ret, char** save)
{
	int i=0, rv=0;
	char	addr[10];
	i = spg761_get_paramaddr(param,TYPE_CONST);
	if (i != -1) {
		sprintf (addr,"%d",i);
		for (i=0; i<ARRAY_SIZE(archive_const761); i++)
		if (archive_const761[i].no==param) {
			rv = spg761_set_string_param(dev, addr, ret, TYPE_CONST, archive_const761[i].type);
			}
		return 0;
    	}
	return -1;
}

static int
make_archive(struct device *dev, int no, AnsLog* alog, int type, struct archive **save)
{
	struct archive *archives = NULL, *lptr = NULL, *ptr;
	int nrecs,i,ret,np=0;
	struct 	tm *tim=malloc (sizeof (struct tm));

	ret = -1;
	nrecs = alog->quant_param;
	if (nrecs <= 0) return -1;
	if (!dev->quiet)
		fprintf(stderr, "RECS: %d\n", nrecs);

	for (i = 0; i < nrecs; ++i) 
		{
		 if ((ptr = alloc_archive()) != NULL) 
			{
			 ptr->num = i;

			 if (type==TYPE_HOURS)
			 for (np=0; np<ARRAY_SIZE(archive_hour761); np++)
				{
				 sscanf(alog->time[archive_hour761[np].id][i],"%d-%d-%d/%d:%d:%d",&tim->tm_mday, &tim->tm_mon, &tim->tm_year, &tim->tm_hour, &tim->tm_min, &tim->tm_sec);
    				 snprintf(ptr->datetime, sizeof(ptr->datetime),"%d-%02d-%02d %02d:%02d:%02d",tim->tm_year+2000, tim->tm_mon, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);
    				 ptr->params[archive_hour761[np].id] = atof (alog->data[archive_hour761[np].id][i]);
    				 //printf ("[%s] %f\n",ptr->datetime,ptr->params[archive_hour761[np].id]);
    				}
    			 if (type==TYPE_DAYS)
			 for (np=0; np<ARRAY_SIZE(archive_day761); np++)
			 //if (archive_day761[np].no==no)
				{
				 sscanf(alog->time[archive_day761[np].id][i],"%d-%d-%d/%d:%d:%d",&tim->tm_mday, &tim->tm_mon, &tim->tm_year, &tim->tm_hour, &tim->tm_min, &tim->tm_sec);
    				 snprintf(ptr->datetime, sizeof(ptr->datetime),"%d-%02d-%02d %02d:%02d:%02d",tim->tm_year+2000, tim->tm_mon, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);
    				 ptr->params[archive_day761[np].id] = atof (alog->data[archive_day761[np].id][i]);
    				 //printf ("[%s] %f\n",ptr->datetime,ptr->params[archive_day761[np].id]);
    				}
    			 if (type==TYPE_MONTH)
			 for (np=0; np<ARRAY_SIZE(archive_month761); np++)
			 //if (archive_month761[np].no==no)
				{
				 sscanf(alog->time[archive_month761[np].id][i],"%d-%d-%d/%d:%d:%d",&tim->tm_mday, &tim->tm_mon, &tim->tm_year, &tim->tm_hour, &tim->tm_min, &tim->tm_sec);
    				 snprintf(ptr->datetime, sizeof(ptr->datetime),"%d-%02d-%02d %02d:%02d:%02d",tim->tm_year+2000, tim->tm_mon, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);
    				 ptr->params[archive_month761[np].id] = atof (alog->data[archive_month761[np].id][i]);
    				 //printf ("[%s] %f\n",ptr->datetime,ptr->params[archive_month761[np].id]);
    				}

			 ret=0;
			 if(!lptr)
				archives = ptr;
			 else
				lptr->next = ptr;
			 lptr = ptr;
			}
		else	{
			ret  = -1;
			break;
		    }
		}
	*save = archives;
	free (tim);
	return ret;
}

static int 
spg761_h_archiv(struct device *dev, int no, const char *from, const char *to, struct archive **save)
{
	struct spg761_ctx *ctx;
	AnsLog alog;
	int ret;
	int i;
	static char ident_query[100], addr[10],	answer[1024];

	struct 	tm *time_from=malloc (sizeof (struct tm));
	ret = sscanf(from,"%d-%d-%d,%d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);
	time_from->tm_mon-=1;

	ctx = get_spec(dev);
	ctx->crc_flag = 0;

	struct 	tm *time_to=malloc (sizeof (struct tm));
	ret = sscanf(to,"%d-%d-%d,%d:%d:%d",&time_to->tm_year, &time_to->tm_mon, &time_to->tm_mday, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);
	time_to->tm_mon-=1;

	for (i=0; i<ARRAY_SIZE(archive_hour761); i++)
//	if (archive_hour761[i].no==no)
		{
		 sprintf (addr,"%d",archive_hour761[i].adr);
		 if (-1 == generate_sequence (dev, TYPE_HOURS, FNC_time_array, addr, 0, 1, no, time_from, time_to, ident_query)) 	break;
	 	 ctx->send_query = ident_query;
	 	 ctx->send_query_sz = strlen(ident_query);
		 if (-1 == dev_query(dev))   break;
		 if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, answer, ANALYSE, archive_hour761[i].id, &alog))		break;
	    	 buf_free(&dev->buf);
	    	 ret = 0;
		}
	if (ret == 0)
	    ret = make_archive(dev, no, &alog, TYPE_HOURS, save);
    	buf_free(&dev->buf);
 	regfree(&ctx->re);
	ctx_zero(ctx);
	free (time_from);
	free (time_to);
	return ret;
}

static int 
spg761_d_archiv(struct device *dev, int no, const char *from, const char *to, struct archive **save)
{
	struct spg761_ctx *ctx;
	AnsLog alog;
	int ret;
	int i;
	static char ident_query[100], addr[10],	answer[1024];

	struct 	tm *time_from=malloc (sizeof (*time_from));
	ret = sscanf(from,"%d-%d-%d %d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);
	time_from->tm_mon-=1;

	ctx = get_spec(dev);
	ctx->crc_flag = 0;

	struct 	tm *time_to=malloc (sizeof (*time_to));
	ret = sscanf(to,"%d-%d-%d %d:%d:%d",&time_to->tm_year, &time_to->tm_mon, &time_to->tm_mday, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);
	time_to->tm_mon-=1;

	for (i=0; i<ARRAY_SIZE(archive_day761); i++)
	if (archive_day761[i].no==no)
		{
		 sprintf (addr,"%d",archive_day761[i].adr);
		 if (-1 == generate_sequence (dev, TYPE_DAYS, FNC_time_array, addr, 0, 1, no, time_from, time_to, ident_query)) 	break;
	 	 ctx->send_query = ident_query;
	 	 ctx->send_query_sz = strlen(ident_query);
		 if (-1 == dev_query(dev))   break;
		 if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, answer, ANALYSE, archive_day761[i].id, &alog))		break;
	    	 buf_free(&dev->buf);
	    	 ret=0;
		}
	if (ret == 0)
	    ret = make_archive(dev, no, &alog, TYPE_DAYS, save);
    	buf_free(&dev->buf);
 	regfree(&ctx->re);
	ctx_zero(ctx);
	free (time_from);
	free (time_to);
	return ret;
}

static int 
spg761_m_archiv(struct device *dev, int no, const char *from, const char *to, struct archive **save)
{
	struct spg761_ctx *ctx;
	AnsLog alog;
	int ret;
	int i;
	static char ident_query[100], addr[10],	answer[1024];

	struct 	tm *time_from=malloc (sizeof (*time_from));
	ret = sscanf(from,"%d-%d-%d %d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);
	time_from->tm_mon-=1;

	ctx = get_spec(dev);
	ctx->crc_flag = 0;

	struct 	tm *time_to=malloc (sizeof (*time_to));
	ret = sscanf(to,"%d-%d-%d %d:%d:%d",&time_to->tm_year, &time_to->tm_mon, &time_to->tm_mday, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);
	time_to->tm_mon-=1;

	for (i=0; i<ARRAY_SIZE(archive_month761); i++)
	if (archive_month761[i].no==no)
		{
		 sprintf (addr,"%d",archive_month761[i].adr);
		 if (-1 == generate_sequence (dev, TYPE_MONTH,FNC_time_array, addr, 0, 1, no, time_from, time_to, ident_query)) 	break;
	 	 ctx->send_query = ident_query;
	 	 ctx->send_query_sz = strlen(ident_query);
		 if (-1 == dev_query(dev))   break;
		 if (-1 == analyse_sequence (dev->buf.p, dev->buf.len, answer, ANALYSE, archive_month761[i].id, &alog))		break;
	    	 buf_free(&dev->buf);
	    	 ret=0;
		}
	if (ret == 0)
	    ret = make_archive(dev, no, &alog, TYPE_MONTH, save);
    	buf_free(&dev->buf);
 	regfree(&ctx->re);
	ctx_zero(ctx);
	free (time_from);
	free (time_to);
	return ret;
}

static int
make_event(char *times, char *events, struct events **save, uint16_t enm)
{
    struct events *ev;
    ev = malloc(sizeof(*ev));
    if (!ev) {
	set_system_error("malloc");
	return -1;
	}
    ev->num = enm; 	// ???
    ev->event = 1;	// ???
    snprintf(ev->datetime, sizeof(ev->datetime),"%s",times);
    ev->next = NULL;
    *save = ev;
    return 1;
}

static int
spg761_events(struct device *dev, const char *from, const char *to, struct events **save)
{
	struct spg761_ctx *ctx;
	AnsLog alog;
	struct events *ev, **pev, *top;
	int ret=1,i;
	static char ident_query[100], answer[1024];

	top = NULL;
	pev = &top;

	ctx = get_spec(dev);
	ctx->crc_flag = 1;

	struct 	tm *time_from=malloc (sizeof (*time_from));
	struct 	tm *time_to=malloc (sizeof (*time_to));

	ret = sscanf(from,"%d-%d-%d %d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);
	time_from->tm_mon-=1;
	ret = sscanf(to,"%d-%d-%d %d:%d:%d",&time_to->tm_year, &time_to->tm_mon, &time_to->tm_mday, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);
	time_to->tm_mon-=1;

	if (-1 == generate_sequence (dev, TYPE_EVENTS, FNC_time_array, "98", 1, 0, 1, time_from, time_to, ident_query)) ret = -1;
 	ctx->send_query = ident_query;
 	ctx->send_query_sz = strlen(ident_query);
	if (ret && -1 == dev_query(dev)) ret = -1;
	if (ret && -1 == analyse_sequence (dev->buf.p, dev->buf.len, answer, ANALYSE, 0, &alog)) ret = -1;

	ev = malloc(sizeof(*ev));
	if (!ev) {
		set_system_error("malloc");
	 	regfree(&ctx->re);
		ctx_zero(ctx);
		ret = -1;
	} 

	wchar_t ans[100];
	if (ret)
	for (i = 0; i < alog.quant_param; i++) {
	        setlocale(LC_CTYPE, "");
		conv_to_utf8 ((unsigned char *)alog.data[0][i], ans);

		sscanf(alog.time[0][i],"%d-%d-%d/%d:%d:%d",&time_from->tm_mday, &time_from->tm_mon, &time_from->tm_year, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);
    		snprintf(alog.time[0][i], sizeof(alog.time[0][i]),"%d-%02d-%02d %02d:%02d:%02d",time_from->tm_year+2000, time_from->tm_mon, time_from->tm_mday, time_from->tm_hour, time_from->tm_min, time_from->tm_sec);

		printf ("[%s][%ls]\n",alog.time[0][i],ans);
		ret = make_event(alog.time[0][i], alog.data[0][i], pev,i);
		if (-1 == ret) {
			free_events(top);
			break;
		}
		pev = &((*pev)->next);
	}

 	regfree(&ctx->re);
 	ctx_zero(ctx);
	free (time_from);
	free (time_to);
	return ret;
}

static int
spg761_send_msg(struct device *dev)
{
	struct spg761_ctx *ctx = get_spec(dev);
	return dev_write(dev, ctx->send_query, ctx->send_query_sz);
}

static int
spg761_parse_msg(struct device *dev, char *p, int psz)
{
    struct spg761_ctx *ctx;
    int ret;
    char answer[1024];
    AnsLog alog;

    ctx = get_spec(dev);

    if (!dev->quiet)
	fprintf(stderr, "\rParsing: %10d bytes", psz);

    if (-1 == analyse_sequence (p, psz, answer, ANALYSE, 0, &alog)) ret = -1;
    else {
	    if (!dev->quiet)
		fprintf(stderr, "\nParsing ok\n");
	    ret = 0;
	    return ret;
	}
    return -1;
}

static int
spg761_parse_crc_msg(struct device *dev, char *p, int psz)
{
    struct spg761_ctx *ctx;
    int ret;
    char answer[1024];
    AnsLog alog;

    ctx = get_spec(dev);

    if (!dev->quiet)
	fprintf(stderr, "\rParsing crc: %10d bytes", psz);

    if (-1 == analyse_sequence (p, psz, answer, NO_ANALYSE, 0, &alog)) {
		ret = -1;
	} else {
		if (!dev->quiet)
			fprintf(stderr, "\nParsing ok\n");
		ctx->crc = alog.crc;
		ret = 0;
	}
	return ret;
}

static int
spg761_check_crc(struct device *dev, char *p, int psz)
{
    struct spg761_ctx *ctx;
    int ret;
    char answer[1024];
    AnsLog alog;

    ctx = get_spec(dev);

    if (ctx->crc_flag) {
		analyse_sequence (p, psz, answer, NO_ANALYSE, 0, &alog);
		return alog.checksym; 
	} else {
		ret = 0;
	}
    return ret;
}

//static int  generate_sequence (struct device *dev, uint8_t type, uint8_t func, const char* padr, uint8_t nchan, uint8_t npipe, int no, struct tm* from, struct tm* to, char* sequence);
// function generate send sequence for logika
static int 
generate_sequence (struct device *dev, uint8_t type, uint8_t func, const char* padr, uint8_t nchan, uint8_t npipe, int no, struct tm* from, struct tm* to, char* sequence)
{
    char buffer[150];
    uint16_t ks=0; 
    switch (func)
	{
	 case 0x88:
		 sprintf (buffer,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",DLE,SOH,0x10,0x1F,0x1D,0x10,0x2,0x9,0x30,0x09,0x30,0x30,0x36,0xC,0x10,0x3); 
	 	 ks = calc_bcc ((uint8_t*)buffer+2, strlen(buffer)-2);
		 sprintf (sequence,"%s%c%c",buffer,(uint8_t)(ks>>8),(uint8_t)(ks%256));
		 break;
	 case FNC_read:
		 sprintf (buffer,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%c%c%c",DLE,SOH,dev->devaddr,0x80,DLE,ISI,func,0x53,0x30,DLE,STX,HT,npipe+48,HT,padr,FF,DLE,ETX); 
	 	 ks = calc_bcc ((uint8_t*)buffer+2, strlen(buffer)-2);
		 sprintf (sequence,"%s%c%c",buffer,(uint8_t)(ks>>8),(uint8_t)(ks%256));
		 break;
 	 case FNC_readindex:
		 sprintf (buffer,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%c%c%c%c%c%c%c",DLE,SOH,dev->devaddr,0x80,DLE,ISI,func,0x53,0x30,DLE,STX,HT,npipe+48,HT,padr,HT,48+nchan,HT,48+no,FF,DLE,ETX);
	 	 ks = calc_bcc ((uint8_t*)buffer+2, strlen(buffer)-2);
		 sprintf (sequence,"%s%c%c",buffer,(uint8_t)(ks>>8),(uint8_t)(ks%256));
		 break;

	 case FNC_write:
		 sprintf (buffer,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%c%c%s%c%c%c",DLE,SOH,dev->devaddr,0x80,DLE,ISI,func,0x53,0x30,DLE,STX,HT,npipe+48,HT,padr,FF,HT,sequence,FF,DLE,ETX);
	 	 ks = calc_bcc ((uint8_t*)buffer+2, strlen(buffer)-2);
		 sprintf (sequence,"%s%c%c",buffer,(uint8_t)(ks>>8),(uint8_t)(ks%256));
		 break;
 	 case FNC_writeindex:
		 sprintf (buffer,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%c%c%c%c%c%c%s%c%c%c",DLE,SOH,dev->devaddr,0x80,DLE,ISI,func,0x53,0x30,DLE,STX,HT,npipe+48,HT,padr,HT,48+nchan,HT,48+1,FF,HT,sequence,FF,DLE,ETX);
	 	 ks = calc_bcc ((uint8_t*)buffer+2, strlen(buffer)-2);
		 sprintf (sequence,"%s%c%c",buffer,(uint8_t)(ks>>8),(uint8_t)(ks%256));
		 break;

	 case FNC_time_array:
		 sprintf (buffer,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%c%c%d%c%d%c%d%c%d%c%d%c%d%c%c%d%c%d%c%d%c%d%c%d%c%d%c%c%c",DLE,SOH,dev->devaddr,0x80,DLE,ISI,func,0x53,0x30,DLE,STX,HT,npipe+48,HT,padr,FF,
			  HT,to->tm_mday,HT,to->tm_mon+1,HT,to->tm_year,HT,to->tm_hour,HT,0,HT,0,FF,HT,from->tm_mday,HT,from->tm_mon+1,HT,from->tm_year,HT,from->tm_hour,HT,0,HT,0,FF,DLE,ETX);
	 	 ks = calc_bcc ((uint8_t*)buffer+2, strlen(buffer)-2);
		 sprintf (sequence,"%s%c%c",buffer,(uint8_t)(ks>>8),(uint8_t)(ks%256));
		 //for (s=0; s<strlen(sequence); s++) printf ("[%d/%d]0x%x\n",s,strlen(sequence),(uint8_t)sequence[s]);
		 break;
	 default:
		 fprintf(stderr, "\runknown function: %d", func); 
		 return -1;
	}
    return 0;
}

//----------------------------------------------------------------------------------------
static int 
analyse_sequence (char* dats, uint len, char* answer, uint8_t analyse, uint8_t id, AnsLog *alog)
{
    char buffer[150];
    char dat[1000];
    if (len>1000) return -1;
    dats[len]=0;
    strncpy (dat,dats,sizeof(dat));
    alog->checksym = 0;
    uint16_t i=0,start=0, startm=0, quant=0;
    for (i=0; i<len; i++)   printf ("in[%d]:0x%x (%c)\n",i,dats[i],dats[i]);
    i=0;
    while (i<len && i<400)
	{
	 if (dat[i]==DLE && dat[i+1]==SOH) 
		{
		 if (dat[i+2]!=DLE)
			{
			 alog->from = (uint8_t)dat[i+3];
			 alog->to = (uint8_t)dat[i+2];
			 startm=i+2; i=i+4;
			}
		 else 
			{
			 i=i+2; startm=i;
			}
		 while (i<len && i<400)
			{
			 if (dat[i]==DLE && dat[i+1]==STX) break;
			 if (dat[i]==DLE && dat[i+1]==ISI)
				{ 
				 alog->func=(uint8_t)dat[i+2];
				 start=i+3; // DataHead
				}
			 i++;
			}
		}

	 if (analyse)
	 if (dat[i]==DLE && dat[i+1]==STX)
		{
		 strncpy (alog->head,dat+start,i-start);
		 i=i+2;
		 if (dat[i]==HT) 
			{
			 alog->pipe = dat[i+1]-48; i=i+2;
			 if (dat[i]==HT)
				{
				 i++; start=i;
				 while (dat[i]!=FF && dat[i]!=HT && i<len) i++; 

				 if (i-start<100) strncpy (buffer,dat+start,i-start); //else fprintf(stderr,"!!!! buffer");
				 buffer[i-start]=0;
				 alog->nadr = atoi (buffer);
				 //fprintf(stderr, "[%d] alog.to [0x%x] alog.from [0x%x] pipe=%d npAdr = %d func = 0x%x",blok,alog.to,alog.from,alog.pipe,alog.npAdr,alog.func);
				 if (!alog->nadr) return -1;

				 alog->from_param = 0;
				 alog->quant_param = 0;
				 if (alog->func==FNC_read)
					{
					 i++; start=i;
					 while (dat[i]!=FF && dat[i]!=HT && i<len) i++;
					 strncpy (answer,dat+start,i-start);
					}
				 if (alog->func==FNC_answer)
					{
					 i++; start=i;
					 while (dat[i]!=FF && dat[i]!=HT && i<len) i++;
					 strncpy (buffer,dat+start,i-start);
					 alog->from_param = atoi (buffer);
					 i++; start=i;
					 while (dat[i]!=FF && dat[i]!=HT && i<len) i++;
					 strncpy (buffer,dat+start,i-start);
					 alog->quant_param = atoi (buffer);
					}
				 if (alog->func==FNC_ptzp) 
				    {
				     return 0;
				    }
				 if (alog->func==FNC_answer_time) 
					{
 					 i++; start=i;
					 while (dat[i]!=FF && i<len) i++;
					 i++;
					 while (dat[i]!=FF && i<len) i++;
					}

	 			 uint16_t pos=0,o=0,j=0,r=0;
				 char datt[5000];
				 char value[ARCHIVE_NUM_MAX][50];
				 if (len>i+9)
					{
					 strncpy (datt,dat+i+2,len-i-7);
					 for (o=0;o<=len-i-7;o++)
						{
						 if (*(dat+i+2+o)==0xc && *(dat+i+3+o)==0x10 && *(dat+i+4+o)==0x3) { datt[o]=0x9; datt[o+1]=0; o=len-i-6;}
						 else datt[o]=(uint8_t)*(dat+i+2+o);
						}
					 datt[o]=0;
					 char* token = strtok(datt,"\t");
					 while(token!=NULL)
						{
						 if (strlen(token)<38) sprintf (value[pos],token);
						 else sprintf (value[pos]," ");
						 //printf("value[%d]=%s\n",pos,value[pos]);
						 token = strtok(NULL,"\t"); pos++;
						}
					}
				 else { value[0][0]=0; value[1][0]=0; value[2][0]=0; }
				 if (alog->func==FNC_answer_time && alog->nadr!=98)
					{
					 uint8_t some_ed=0, no_ed=0;
					 if (strlen(value[4])>14 && strlen(value[2])>14)
						 some_ed=1;
					 if (strlen(value[1])>14 && alog->nadr>100) no_ed=1;
					 for (j=0,r=0;j<pos;j++)
						{
						 if (pos<2 || (!strlen(value[j]) && !strlen(value[j+1]))) continue;
						 strcpy (alog->data[id][r],value[j]);
						 j++;
						 if (!no_ed)
							{
							 if (some_ed) strcpy (alog->type[id][r],value[1]);
							 if (!some_ed) { strcpy (alog->type[id][r],value[j]);	j++;}
							 if (j==1 && some_ed) j++;
							}
						 else strcpy (alog->type[id][r],"-");
						 if (strlen(value[j])>27) 
							{
							 strcpy (alog->time[id][r],"");
							}
						 else strcpy (alog->time[id][r],value[j]);

						 uint16_t ln = strlen(alog->time[id][r]);
						 alog->time[id][r][ln-1]=0;
						 //printf("arch [%s] (%s) %s\n",alog->time[id][r],alog->type[id][r],alog->data[id][r]);

						 if (strlen(alog->time[id][r])<10) break;
						 if (strlen(alog->type[id][r])>10) break;
						 r++; quant++;
						}
					 alog->quant_param = quant;
					}
				if (alog->func==FNC_answer_time && alog->nadr==98)
				    {
				     quant=0;
				     for (j=0,r=0;j<pos;j++)
					{
					 snprintf (alog->data[id][r],68,"%s %s",value[j],value[j+1]);
					 snprintf (alog->time[id][r],28,"%s",value[j+2]);
					 snprintf (alog->type[id][r],28,"-");
					 //printf("arch [%s] (%s) %s\n",alog->time[id][r],alog->type[id][r],alog->data[id][r]);
					 r++; quant++; j+=2;
					}
    				     alog->quant_param = quant;
				    }

				if (alog->func==FNC_answer || alog->func==FNC_write)
					{
					 alog->quant_param=1;
					 for (j=0; j<strlen(value[0]);j++)
						if (value[0][j]==0x3F)
							alog->quant_param=0;
					 if (alog->quant_param)	{
						strncpy (alog->data[id][0],value[0],37);
						strncpy (answer,value[0],37);
						strncpy (alog->type[id][0],value[1],17);
						}
					    else	{
						sprintf (alog->data[id][0],"no data");
						alog->type[id][0][0]=0;
						alog->time[id][0][0]=0;
					    }
					}
				}
			 else return -1;
			}
		 else return -1;
		}
	 if (dat[i]==DLE && dat[i+1]==ETX)
		{
		 uint16_t ks=calc_bcc ((uint8_t*)dat+startm,len-startm-2);
		 //fprintf(stderr,"KS [%d->%d] %x %x\n",startm,len-startm-2,(uint8_t)(ks/256),(uint8_t)dat[i+2]);
		 if (((uint8_t)(ks/256)==(uint8_t)dat[i+2])&&((uint8_t)(ks%256)==(uint8_t)dat[i+3]))
			{
			 alog->checksym = ks;
			 return 0;
			}
		 else alog->checksym = 0;
		 return -1;
		}
	 i++;
	}
    return -1;
}
//----------------------------------------------------------------------------------------
static int 
conv_to_utf8 (unsigned char* dat, wchar_t* utf)
{
    int i=0;
    for (i=0;i<strlen((char*)dat);i++)	{
        utf[i]=dat[i];
        if (dat[i]>=128 && dat[i]<=175) utf[i]=0x410+(dat[i]-128);
        if (dat[i]>=224 && dat[i]<=239) utf[i]=0x440+(dat[i]-224);
    }
    utf[i]=0;
    return 0;
}
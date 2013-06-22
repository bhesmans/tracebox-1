/*
 *  Copyright (C) 2013  Gregory Detal <gregory.detal@uclouvain.be>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301, USA.
 */

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
};

#include "crafter.h"
#include "crafter/Utils/IPResolver.h"
#include "script.h"

#include <cstdlib>
#include <ctime>
#include <climits>

using namespace Crafter;

/* Dummy class to facilitate macros */
class Raw : public RawLayer { };

static const char *layer_names[] = {
	"IP", "IPv6", "TCP", "UDP", "Raw",
	"TCPOptionLayer", "IPOptionLayer",
	NULL
};
static const char *pkt_names[] = {"Packet", NULL};

void *__checkudata(lua_State *l, int i, const char *name)
{
	void *udata = NULL;

	luaL_checktype(l, i, LUA_TUSERDATA);
	lua_getmetatable(l, i);
	luaL_getmetatable(l, name);
	if (lua_equal(l, -1, -2))
		udata = *(void **)lua_touserdata(l, i);
        lua_pop(l, 2);
	return udata;
}

void *luaL_checkudata_ex(lua_State *l, int i, const char **names)
{
	void *udata;

	while (*names)
		if (udata = __checkudata(l, i, *names++))
			return udata;
	return NULL;
}

#define l_new_(obj, obj2) \
static obj2 *l_##obj2##_new(lua_State *l) \
{ \
	 obj2 **udata = (obj2 **)lua_newuserdata(l, sizeof(obj2 *)); \
	*udata = new obj2(); \
	luaL_getmetatable(l, #obj); \
	lua_setmetatable(l, -2); \
	return *udata; \
}

#define l_new(obj) \
	l_new_(obj, obj);

#define l_check(obj) \
static obj *l_##obj##_check(lua_State *l, int n) \
{ \
	return *(obj **)luaL_checkudata(l, n, #obj); \
}

#define l_constructor(obj) \
l_new(obj); \
static int l_##obj##_constructor(lua_State *l) \
{ \
	return l_##obj##_new(l) != NULL; \
}

#define l_destructor(obj) \
static int l_##obj##_destructor(lua_State *l) \
{ \
    obj *o= l_##obj##_check(l, 1); \
    delete o; \
    return 1; \
}

#define l_hexdump(obj) \
static int l_##obj##_hexdump(lua_State *l) \
{ \
	std::ostringstream stream; \
	obj *o = (obj *)l_##obj##_check(l, 1); \
	o->HexDump(stream); \
	lua_pushstring(l, stream.str().c_str()); \
	return 1; \
}

#define l_print(obj) \
static int l_##obj##_print(lua_State *l) \
{ \
	std::ostringstream stream; \
	obj *o = (obj *)l_##obj##_check(l, 1); \
	o->Print(stream); \
	lua_pushstring(l, stream.str().c_str()); \
	return 1; \
}

#define l_setter(obj, f, t) \
static int l_##obj##_Set##f(lua_State *l) \
{ \
	obj *o= l_##obj##_check(l, 1); \
	o->Set##f(luaL_check##t(l, 2)); \
	return 1; \
}

#define l_constant(l, v, n) \
	lua_pushnumber(l, v); \
	lua_setglobal(l, #n);

#define l_register(l, obj, reg) \
	luaL_newmetatable(l, #obj); \
	luaL_register(l, NULL, reg); \
	lua_pushvalue(l, -1); \
	lua_setfield(l, -1, "__index"); \
	lua_setglobal(l,  #obj);

l_check(Packet);
l_new(Packet);
l_destructor(Packet);
l_print(Packet);
l_hexdump(Packet);

l_check(IP);
l_constructor(IP);
l_destructor(IP);
l_print(IP);
l_hexdump(IP);
l_setter(IP, SourceIP, string);
l_setter(IP, DestinationIP, string);
l_setter(IP, Flags, number);
l_setter(IP, FragmentOffset, number);
l_setter(IP, TTL, number);
l_setter(IP, Identification, number);
l_setter(IP, DiffServicesCP, number);
l_setter(IP, ExpCongestionNot, number);

l_check(IPOptionLayer);
l_destructor(IPOptionLayer);
l_print(IPOptionLayer);
l_hexdump(IPOptionLayer);
l_new_(IPOptionLayer, IPOptionRR);
l_new_(IPOptionLayer, IPOptionLSRR);
l_new_(IPOptionLayer, IPOptionSSRR);
l_new_(IPOptionLayer, IPOptionPad);
l_new_(IPOptionLayer, IPOptionTraceroute);

l_check(IPv6);
l_constructor(IPv6);
l_destructor(IPv6);
l_print(IPv6);
l_hexdump(IPv6);
l_setter(IPv6, SourceIP, string);
l_setter(IPv6, DestinationIP, string);
l_setter(IPv6, TrafficClass, number);
l_setter(IPv6, FlowLabel, number);
l_setter(IPv6, HopLimit, number);

l_check(TCP);
l_constructor(TCP);
l_destructor(TCP);
l_print(TCP);
l_hexdump(TCP);
l_setter(TCP, SrcPort, number);
l_setter(TCP, DstPort, number);
l_setter(TCP, SeqNumber, number);
l_setter(TCP, AckNumber, number);
l_setter(TCP, WindowsSize, number);
l_setter(TCP, Flags, number);

l_check(TCPOptionLayer);
l_destructor(TCPOptionLayer);
l_print(TCPOptionLayer);
l_hexdump(TCPOptionLayer);
l_setter(TCPOptionLayer, Kind, number);
l_setter(TCPOptionLayer, Payload, string);
l_new_(TCPOptionLayer, TCPOptionSACKPermitted);
l_new_(TCPOptionLayer, TCPOptionSACK);
l_new_(TCPOptionLayer, TCPOptionMaxSegSize);
l_new_(TCPOptionLayer, TCPOptionTimestamp);
l_new_(TCPOptionLayer, TCPOptionPad);
l_new_(TCPOptionLayer, TCPOption);

l_check(UDP);
l_constructor(UDP);
l_destructor(UDP);
l_print(UDP);
l_hexdump(UDP);
l_setter(UDP, SrcPort, number);
l_setter(UDP, DstPort, number);

l_check(Raw);
l_constructor(Raw);
l_destructor(Raw);
l_print(Raw);
l_hexdump(Raw);
l_setter(Raw, Payload, string);

static int l_concat(lua_State *l)
{
	Layer *l1 = (Layer *)luaL_checkudata_ex(l, 1, layer_names);
	Packet *p1 = (Packet *)luaL_checkudata_ex(l, 1, pkt_names);
	Layer *l2 = (Layer *)luaL_checkudata_ex(l, 2, layer_names);
	Packet *p2 = (Packet *)luaL_checkudata_ex(l, 2, pkt_names);

	if (l1 && l2) {
		Packet *pkt = l_Packet_new(l);
		pkt->PushLayer(*l1);
		pkt->PushLayer(*l2);
	} else if (p1 && l2) {
		p1->PushLayer(*l2);
		p1->PreCraft();
		lua_pop(l, 1);
	} else if (l1 && p2) {
		Packet *pkt = l_Packet_new(l);
		pkt->PushLayer(*l1);
		*pkt /= *p2;
		pkt->PreCraft();
	} else if (p1 && p2){
		*p1 /= *p2;
		lua_pop(l, 1);
	} else {
		return 0;
	}
	return 1;
}

#define l_defunc_(obj) \
	{ "__gc", l_##obj##_destructor }, \
	{ "__tostring", l_##obj##_print }, \
	{ "__concat", l_concat }, \
	{ "__add", l_concat }, \
	{ "__div", l_concat }, \
	{ "hexdump", l_##obj##_hexdump }

#define l_defunc(obj) \
	{ "new", l_##obj##_constructor }, \
	l_defunc_(obj)

static luaL_Reg sPacketRegs[] = {
	l_defunc_(Packet),
	{ NULL, NULL }
};

static luaL_Reg sIPRegs[] = {
	l_defunc(IP),
	{ "source", l_IP_SetSourceIP },
	{ "dest", l_IP_SetDestinationIP },
	{ "flags", l_IP_SetFlags },
	{ "fragoffset", l_IP_SetFragmentOffset },
	{ "ttl", l_IP_SetTTL },
	{ "id", l_IP_SetIdentification },
	{ "dscp", l_IP_SetDiffServicesCP },
	{ "ecn", l_IP_SetExpCongestionNot },
	{ NULL, NULL }
};

static luaL_Reg sIPOptionRegs[] = {
	l_defunc_(IPOptionLayer),
	{ NULL, NULL }
};

static luaL_Reg sIPv6Regs[] = {
	l_defunc(IPv6),
	{ "source", l_IPv6_SetSourceIP },
	{ "dest", l_IPv6_SetDestinationIP },
	{ "tc", l_IPv6_SetTrafficClass },
	{ "flowlabel", l_IPv6_SetFlowLabel },
	{ "hoplimit", l_IPv6_SetHopLimit },
	{ NULL, NULL }
};

static luaL_Reg sTCPRegs[] = {
	l_defunc(TCP),
	{ "source", l_TCP_SetSrcPort },
	{ "dest", l_TCP_SetDstPort },
	{ "seq", l_TCP_SetSeqNumber },
	{ "ack", l_TCP_SetAckNumber },
	{ "win", l_TCP_SetWindowsSize },
	{ "flags", l_TCP_SetFlags },
	{ NULL, NULL }
};

static luaL_Reg sTCPOptionRegs[] = {
	l_defunc_(TCPOptionLayer),
	{ "kind", l_TCPOptionLayer_SetKind },
	{ "data", l_TCPOptionLayer_SetPayload },
	{ NULL, NULL }
};

static luaL_Reg sUDPRegs[] = {
	l_defunc(UDP),
	{ "source", l_UDP_SetSrcPort },
	{ "dest", l_UDP_SetDstPort },
	{ NULL, NULL }
};

static luaL_Reg sRawRegs[] = {
	l_defunc(Raw),
	{ "data", l_Raw_SetPayload },
	{ NULL, NULL }
};

static int v_arg(lua_State* L, int argt, const char* field)
{
	luaL_checktype(L, argt, LUA_TTABLE);

	lua_getfield(L, argt, field);

	if(lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return 0;
	}
	return lua_gettop(L);
}

static const char* v_arg_lstring(lua_State* L, int argt, const char* field, size_t* size, const char* def)
{
	if(!v_arg(L, argt, field))
	{
		if(def) {
			lua_pushstring(L, def);
			return lua_tolstring(L, -1, size);
		} else {
			const char* msg = lua_pushfstring(L, "%s is missing", field);
			luaL_argerror(L, argt, msg);
		}
	}

	if(!lua_tostring(L, -1)) {
		const char* msg = lua_pushfstring(L, "%s is not a string", field);
		luaL_argerror(L, argt, msg);
	}

	return lua_tolstring(L, -1, size);
}

static void v_arg_lstring_(lua_State* L, int argt, const char *field, const char **val)
{
	if(!lua_tostring(L, -1)) {
		const char* msg = lua_pushfstring(L, "%s is not a string", field);
		luaL_argerror(L, argt, msg);
	}

	*val = lua_tolstring(L, -1, NULL);
}

static const char* v_arg_string(lua_State* L, int argt, const char* field)
{
	return v_arg_lstring(L, argt, field, NULL, NULL);
}

static bool v_arg_string_opt(lua_State* L, int argt, const char* field, const char **val)
{
	if(!v_arg(L, argt, field))
		return false;
	v_arg_lstring_(L, argt, field, val);
	return true;
}

static int v_arg_integer_get_(lua_State* L, int argt, const char* field)
{
	if(lua_type(L, -1) != LUA_TNUMBER) {
		const char* msg = lua_pushfstring(L, "%s is not an integer", field);
		luaL_argerror(L, argt, msg);
	}

	return lua_tointeger(L, -1);
}

static int v_arg_integer(lua_State* L, int argt, const char* field)
{
	if(!v_arg(L, argt, field))
	{
		const char* msg = lua_pushfstring(L, "%s is missing", field);
		luaL_argerror(L, argt, msg);
	}

	return v_arg_integer_get_(L, argt, field);
}

static bool v_arg_integer_opt(lua_State* L, int argt, const char* field, int *val)
{
	if(!v_arg(L, argt, field))
		return false;

	*val = v_arg_integer_get_(L, argt, field);
	return true;
}

static int l_IP(lua_State *l)
{
	IP *ip;
	const char *dst;
	int id, flags, ttl, dscp, fragoffset, ecn;
	bool dst_set = v_arg_string_opt(l, 1, "dst", &dst);
	bool id_set = v_arg_integer_opt(l, 1, "id", &id);
	bool flags_set = v_arg_integer_opt(l, 1, "flags", &flags);
	bool ttl_set = v_arg_integer_opt(l, 1, "ttl", &ttl);
	bool fragoffset_set = v_arg_integer_opt(l, 1, "offset", &fragoffset);
	bool ecn_set = v_arg_integer_opt(l, 1, "ecn", &ecn);
	bool dscp_set = v_arg_integer_opt(l, 1, "dscp", &dscp);

	ip = l_IP_new(l);
	if (!ip)
		return 0;
	if (dst_set)
		ip->SetDestinationIP(dst);
	if (id_set)
		ip->SetIdentification(id);
#ifdef __APPLE__
	else /* FreeBSD add random IP ID if not set */
		ip->SetIdentification(rand() % USHRT_MAX);
#endif
	if (ttl_set)
		ip->SetTTL(ttl);
	if (flags_set)
		ip->SetFlags(flags);
	if (fragoffset_set)
		ip->SetFragmentOffset(fragoffset);
	if (ecn_set)
		ip->SetExpCongestionNot(ecn);
	if (dscp_set)
		ip->SetDiffServicesCP(dscp);
	return 1;
}

static int l_IP_NOP(lua_State *l)
{
	IPOptionPad *opt = l_IPOptionPad_new(l);
	if (opt == NULL)
		return 0;
	opt->SetOption(1);
	return 1;
}

static int l_IP_EOL(lua_State *l)
{
	IPOptionPad *opt = l_IPOptionPad_new(l);
	if (opt == NULL)
		return 0;
	opt->SetOption(0);
	return 1;
}

static int l_IP_SSRR(lua_State *l)
{
	IPOptionSSRR *opt;
	std::vector<std::string> ips;

	if (!lua_istable(l, 1)) {
		const char* msg = lua_pushfstring(l, "argument must be a table");
		luaL_argerror(l, 1, msg);
		return 0;
	}

	lua_pushnil(l);
	while (lua_next(l, 1)) {
		const char *ip = luaL_checkstring(l, -1);
		ips.push_back(ip);
		lua_pop(l, 1);
	}

	opt = l_IPOptionSSRR_new(l);
	if (!opt)
		return 0;

	opt->SetPointer(4);
	/* Put the raw IPs data in the optoin payload */
	opt->SetPayload(IPtoRawData(ips));

	return 1;
}

static int l_IP_LSRR(lua_State *l)
{
	IPOptionLSRR *opt;
	std::vector<std::string> ips;

	if (!lua_istable(l, 1)) {
		const char* msg = lua_pushfstring(l, "argument must be a table");
		luaL_argerror(l, 1, msg);
		return 0;
	}

	lua_pushnil(l);
	while (lua_next(l, 1)) {
		const char *ip = luaL_checkstring(l, -1);
		ips.push_back(ip);
		lua_pop(l, 1);
	}

	opt = l_IPOptionLSRR_new(l);
	if (!opt)
		return 0;

	opt->SetPointer(4);
	/* Put the raw IPs data in the optoin payload */
	opt->SetPayload(IPtoRawData(ips));

	return 1;
}

static int l_IP_RR(lua_State *l)
{
	IPOptionRR *opt;
	int n = luaL_checknumber(l, 1);
	std::vector<std::string> ips(n, "0.0.0.0");

	opt = l_IPOptionRR_new(l);
	if (!opt)
		return 0;

	opt->SetPointer(4);
	/* Put the raw IPs data in the optoin payload */
	opt->SetPayload(IPtoRawData(ips));

	return 1;
}

static int l_IP_Traceroute(lua_State *l)
{
	IPOptionTraceroute *opt;
	const char *src = luaL_checkstring(l, 1);

	opt = l_IPOptionTraceroute_new(l);
	if (!opt)
		return 0;

	opt->SetIDNumber(rand() % USHRT_MAX);
	opt->SetOrigIP(src);

	return 1;
}

static int l_IPv6(lua_State *l)
{
	IPv6 *ipv6;
	const char* dst = v_arg_string(l, 1, "dst");
	int tc, flabel, hoplimit;
	bool tc_set = v_arg_integer_opt(l, 1, "tc", &tc);
	bool flabel_set = v_arg_integer_opt(l, 1, "flowlabel", &flabel);
	bool hoplimit_set = v_arg_integer_opt(l, 1, "hoplimit", &hoplimit);

	ipv6 = l_IPv6_new(l);
	if (!ipv6)
		return 0;

	ipv6->SetDestinationIP(dst);
	if (tc_set)
		ipv6->SetTrafficClass(tc);
	if (hoplimit_set)
		ipv6->SetHopLimit(hoplimit);
	if (flabel_set)
		ipv6->SetFlowLabel(flabel);
	return 1;
}

static int l_TCP(lua_State *l)
{
	TCP *tcp;
	int src, dst, seq, ack, win, flags;
	bool src_set = v_arg_integer_opt(l, 1, "src", &src);
	bool dst_set = v_arg_integer_opt(l, 1, "dst", &dst);
	bool seq_set = v_arg_integer_opt(l, 1, "seq", &seq);
	bool ack_set = v_arg_integer_opt(l, 1, "ack", &ack);
	bool win_set = v_arg_integer_opt(l, 1, "win", &win);
	bool flags_set = v_arg_integer_opt(l, 1, "flags", &flags);

	tcp = l_TCP_new(l);
	if (!tcp)
		return 0;

	tcp->SetSrcPort(src_set ? src : rand() % USHRT_MAX);
	tcp->SetDstPort(dst_set ? dst : rand() % USHRT_MAX);
	tcp->SetSeqNumber(seq_set ? seq : rand() % UINT_MAX);
	if (ack_set)
		tcp->SetAckNumber(ack);
	if (win_set)
		tcp->SetWindowsSize(win);
	tcp->SetFlags(flags_set ? flags : TCP::SYN);
	return 1;
}

static int l_TCP_NOP(lua_State *l)
{
	TCPOptionPad *opt = l_TCPOptionPad_new(l);
	if (opt == NULL)
		return 0;
	opt->SetKind(1);
	return 1;
}

static int l_TCP_EOL(lua_State *l)
{
	TCPOptionPad *opt = l_TCPOptionPad_new(l);
	if (opt == NULL)
		return 0;
	opt->SetKind(0);
	return 1;
}

static int l_TCP_SACKP(lua_State *l)
{
	TCPOptionSACKPermitted *opt = l_TCPOptionSACKPermitted_new(l);
	return opt != NULL;
}

static TCPOptionSACK::Pair extract_pair(lua_State *l, int i)
{
	int left, right;

	// lua_next has already been called
	left = luaL_checknumber(l, -1);
	lua_pop(l, 1);
	lua_next(l, i);
	right = luaL_checknumber(l, -1);
	lua_pop(l, 1);

	return TCPOptionSACK::Pair(left, right);
}

static int l_TCP_SACK(lua_State *l)
{
	TCPOptionSACK *opt;
	std::vector<TCPOptionSACK::Pair> v;

	if (!lua_istable(l, 1)) {
		const char* msg = lua_pushfstring(l, "argument must be a table");
		luaL_argerror(l, 1, msg);
		return 0;
	}
	lua_pushnil(l);
	while (lua_next(l, 1)) {
		int i = lua_gettop(l);

		if (lua_isnumber(l, -1)) {
			v.push_back(extract_pair(l, 1));
		} else if (lua_istable(l, -1)) {
			lua_pushnil(l);
			lua_next(l, i);
			v.push_back(extract_pair(l, i));
			lua_pop(l, 2);
		} else {
			const char* msg = lua_pushfstring(l, "element must be a table");
			luaL_argerror(l, -1, msg);
			return 0;
		}
	}

	opt = l_TCPOptionSACK_new(l);
	if (!opt)
		return 0;

	opt->SetBlocks(v);

	return 1;
}

static int l_TCP_MSS(lua_State *l)
{
	TCPOptionMaxSegSize *opt;
	int mss = luaL_checknumber(l, 1);

	opt = l_TCPOptionMaxSegSize_new(l);
	if (!opt)
		return 0;

	opt->SetMaxSegSize(mss);
	return 1;
}

static int l_TCP_Timestamp(lua_State *l)
{
	TCPOptionTimestamp *opt;
	int val, ecr;
	bool val_set = v_arg_integer_opt(l, 1, "val", &val);
	bool ecr_set = v_arg_integer_opt(l, 1, "ecr", &ecr);

	opt = l_TCPOptionTimestamp_new(l);
	if (!opt)
		return 0;

	opt->SetValue(val_set ? val : rand() % UINT_MAX);
	opt->SetEchoReply(ecr_set ? ecr : 0);

	return 1;
}

static int l_TCP_WindowScale(lua_State *l)
{
	TCPOption *opt;
	const char shift[] = {luaL_checknumber(l, 1), 0};

	opt = l_TCPOption_new(l);
	if (!opt)
		return 0;

	opt->SetKind(3);
	opt->SetPayload(shift);

	return 1;
}

static int l_UDP(lua_State *l)
{
	UDP *udp;
	int src, dst;
	bool src_set = v_arg_integer_opt(l, 1, "src", &src);
	bool dst_set = v_arg_integer_opt(l, 1, "dst", &dst);

	udp = l_UDP_new(l);
	if (!udp)
		return 0;

	udp->SetSrcPort(src_set ? src : rand() % USHRT_MAX);
	udp->SetSrcPort(dst_set ? dst : rand() % USHRT_MAX);
	return 1;
}

static int l_Raw(lua_State *l)
{
	Raw *raw;
	const char *payload = luaL_checkstring(l, 1);

	raw = l_Raw_new(l);
	if (!raw)
		return 0;

	raw->SetPayload(payload);
	return 1;
}

static lua_State *l_init()
{
	lua_State * l = luaL_newstate();
	luaL_openlibs(l);

	/* disable libcrafter warnings */
	ShowWarnings = 0;

	lua_register(l, "ip", l_IP);
	lua_register(l, "ipv6", l_IPv6);
	lua_register(l, "tcp", l_TCP);
	lua_register(l, "udp", l_UDP);
	lua_register(l, "raw", l_Raw);

	l_register(l, Packet, sPacketRegs);
	l_register(l, IP, sIPRegs);
	l_register(l, IPv6, sIPv6Regs);
	l_register(l, TCP, sTCPRegs);
	l_register(l, UDP, sUDPRegs);
	l_register(l, Raw, sRawRegs);

	/* IP options */
	l_register(l, IPOptionLayer, sIPOptionRegs);
	lua_register(l, "ip_nop", l_IP_NOP);
	lua_register(l, "ip_eol", l_IP_EOL);
	lua_register(l, "rr", l_IP_RR);
	lua_register(l, "ssrr", l_IP_SSRR);
	lua_register(l, "lsrr", l_IP_LSRR);
	lua_register(l, "traceroute", l_IP_Traceroute);
	luaL_dostring(l, "IP_NOP=ip_nop()");
	luaL_dostring(l, "IP_EOL=ip_eol()");
	luaL_dostring(l, "function RR(n) return rr(n)/IP_NOP end");
	luaL_dostring(l, "function SSRR(addrs) return ssrr(addrs)/IP_NOP end");
	luaL_dostring(l, "function LSRR(addrs) return lsrr(addrs)/IP_NOP end");

	/* TCP options */
	l_register(l, TCPOptionLayer, sTCPOptionRegs);
	lua_register(l, "nop", l_TCP_NOP);
	lua_register(l, "eol", l_TCP_EOL);
	lua_register(l, "sackp", l_TCP_SACKP);
	lua_register(l, "sack", l_TCP_SACK);
	lua_register(l, "mss", l_TCP_MSS);
	lua_register(l, "timestamp", l_TCP_Timestamp);
	lua_register(l, "wscale", l_TCP_WindowScale);
	luaL_dostring(l, "NOP=nop()");
	luaL_dostring(l, "EOL=eol()");
	luaL_dostring(l, "SACKP=NOP/NOP/sackp()");
	luaL_dostring(l, "MSS=mss(1460)");
	luaL_dostring(l, "TS=NOP/NOP/timestamp{}");
	luaL_dostring(l, "function SACK(blocks) return NOP/NOP/sack(blocks) end");
	luaL_dostring(l, "WSCALE=wscale(14)/NOP");

	return l;
}

Packet *script_packet(std::string& cmd)
{
	int ret;
	std::string command = "pkt=" + cmd;

	lua_State *l = l_init();
	ret = luaL_dostring(l, command.c_str());
	if(ret) {
		std::cout << "Lua error: " << luaL_checkstring(l, -1) << std::endl;
		return NULL;
	}

	lua_getglobal(l, "pkt");
	Packet *pkt = l_Packet_check(l, -1);
	if (!pkt)
		return NULL;
	
	return pkt;
}

void script_execfile(std::string& filename)
{
	int ret;

	lua_State *l = l_init();
	ret = luaL_dofile(l, filename.c_str());
	if(ret)
		std::cout << "Lua error: " << luaL_checkstring(l, -1) << std::endl;
}
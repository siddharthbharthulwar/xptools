#include <vector>
#include <set>
#include <string>
#include <math.h>
#include <stdio.h>

using namespace::std;

void	copy_string_vector(const char ** i, vector<string>& o)
{
	while(*i)
	{
		o.push_back(*i);
		++i;
	}
}


//---------------------------------------------------------------------------------------------------------------------------------------------------

enum surface_type { 
	none,
	grass,
	gravel,
	asphalt
};

const char * surf_name(surface_type t) 
{
	switch(t) {
	case none: return ""; 
	case asphalt: return "asphalt";
	case grass: return "grass";
	case gravel: return "gravel";
	}
}

enum grading_type {
	draped,
	graded
};


//---------------------------------------------------------------------------------------------------------------------------------------------------

class	shader {
public:

	shader(const char * tex_name, int poly_os, int w) : width(w), tex(tex_name), os(poly_os), 
		tile_x(1), tile_y(1), bias_x(1.0), bias_y(1.0), wave_x(1.0), wave_y(1.0)
		{ idx = all.size(); all.push_back(this); }
	void set_tile(int tx, int ty, float bx, float by, float wx, float wy) 
		{ tile_x = tx; tile_y = ty; bias_x = bx; bias_y = by; wave_x = wx; wave_y = wy; }
	
	static void output(FILE * fi) { for (vector<shader*>::iterator s = all.begin(); s != all.end(); ++s) (*s)->output_one(fi); }

	static vector<shader*> all;
	string tex;
	int os;
	int idx;
	int tile_x, tile_y;
	float bias_x, bias_y;
	int wave_x, wave_y;
	int width;
	
	void output_one(FILE * fi)
	{
		if(tile_x < 1) printf("WARNING: shader %s has X tile param of %d\n", tex.c_str(),tile_x);
		if(tile_y < 1) printf("WARNING: shader %s has Y tile param of %d\n", tex.c_str(),tile_y);
		if(bias_x < 0.0 || bias_x > 1.0) printf("WARNING: shader %s has tile x bias %f\n", tex.c_str(), bias_x);
		if(bias_y < 0.0 || bias_y > 1.0) printf("WARNING: shader %s has tile x bias %f\n", tex.c_str(), bias_y);
		fprintf(fi,"# Shader %d\n", idx);
		fprintf(fi,"TEXTURE\t%d %s\n", os, tex.c_str());
		if(tile_x != 1 || tile_y != 1)
			fprintf(fi,"TEXTURE_TILE\t%d %d %.3f %.3f %.1f %.1f\n",tile_x,tile_y,bias_x, bias_y, wave_x, wave_y);
		fprintf(fi,"\n");
	}
};

vector<shader *> shader::all;

static void set_shader_width(FILE * fi, shader * s)
{
	static shader * last = NULL;
	if(last == NULL || last->width != s->width)
	{
		fprintf(fi,"SCALE %d\n", s->width);
	}
	last = s;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

struct	lod_range {
	int		near;
	int		far;
	
	bool operator==(const lod_range& rhs) const { return near == rhs.near && far == rhs.far; }
};

struct vert_props {
	float	length;
	float	cut;
	
	bool operator==(const vert_props& rhs) const { return length == rhs.length && cut == rhs.cut; }
};

struct perlin_params {
	float	w[4];
	float	a[4];
	bool is_null(void) const { return a[0] == 0.0 && a[1] == 0.0 && a[2] == 0.0 && a[3] == 0.0; }
};

perlin_params def_perlin = { 0 };

struct road_center_marker {
	float			x;
};

struct road_segment {
	shader *		shad;
	surface_type	surf;
	grading_type	grad;
	float			x[2];
	float			y[2];
	float			s[2];
	lod_range		lod;
	vert_props		length;
	
	inline float	width() const { return fabs(x[1] - x[0]); }	
	inline float	max_x() const { return max(x[0],x[1]); }
	road_segment&	operator+=(float o) { x[0] += o; x[1] += o; return *this; }
	
	// This answers the question: if we put me and then other end to end does my p2 match their p1.
	bool	can_optimize_with(const road_segment& rhs) const
	{
		if( shad == rhs.shad &&							// All of these props must be an exact mach
			   grad == rhs.grad &&
			   lod == rhs.lod &&
			   length == rhs.length &&
			   x[1] == rhs.x[0] &&							// Our second point must match their first point exactly.
			   y[1] == rhs.y[0] &&
			   s[1] == rhs.s[0] &&
			   y[1] == y[0] &&								// BOTH segments must be horizontal
			   rhs.y[1] == rhs.y[0] &&
			   (x[1] > x[0]) == (rhs.x[1] > rhs.x[0]))		// BOTH must be directed in the SAME direction.			   
		{
			return true;
		}
		return false;
	}
	
	void	do_optimize_with(const road_segment& rhs)
	{
		x[1] = rhs.x[1];
		y[1] = rhs.y[1];
		s[1] = rhs.s[1];
		surf = max(surf,rhs.surf);
	}
	
};

struct obj_placement {
	vector<string>	models;
	float	lat[2];
	float	psi[2];
	float	off[2];
	float	spa[2];
	float	frq[2];
	perlin_params	wave;
	grading_type	grad;
};


class	road {
public:

	inline float max_x(void) const { 
		if(segments.empty()) return 0.0f;
		float m = segments[0].max_x();
		for(int n = 1; n < segments.size(); ++n)
			m = max(segments[n].max_x(),m);
		return m;
	}

	road& operator+=(const road& rhs)
	{
		assert(markers.empty() || rhs.markers.empty());
		float o = max_x();
		int n = segments.size();
		segments.insert(segments.end(),rhs.segments.begin(),rhs.segments.end());
		for(int i = n; i < segments.size(); ++i)
			segments[i] += o;
			
		n = markers.size();
		markers.insert(markers.end(),rhs.markers.begin(),rhs.markers.end());
		for(int i = n; i < markers.size(); ++i)
			markers[i].x += o;
			
		n = objects.size();
		objects.insert(objects.end(),rhs.objects.begin(),rhs.objects.end());
		for(int i = n; i < objects.size(); ++i)
		{
			objects[i].lat[0] += o;
			objects[i].lat[1] += o;
		}
		
		return *this;
	}

	road& operator|=(const road& rhs)
	{
		segments.insert(segments.end(),rhs.segments.begin(),rhs.segments.end());
		assert(markers.empty() || rhs.markers.empty());
		markers.insert(markers.end(),rhs.markers.begin(),rhs.markers.end());
		objects.insert(objects.end(),rhs.objects.begin(),rhs.objects.end());
		return *this;
	}
	
	road& operator+=(float o)
	{
		for(int i = 0; i < segments.size(); ++i)
			segments[i] += o;
		for(int i = 0; i < markers.size(); ++i)
			markers[i].x += o;
		return *this;
	}

	road operator+(const road& rhs) const { road ret(*this); ret += rhs; return ret; }
	road operator|(const road& rhs) const { road ret(*this); ret |= rhs; return ret; }

	vector<road_segment>		segments;
	vector<obj_placement>		objects;
	vector<road_center_marker>	markers;
	
	void	set_center_at(float x)
	{
		if(markers.empty()) markers.push_back(road_center_marker());
		markers[0].x = x;
	}
	void	set_center_at_center(void)
	{
		set_center_at(max_x() * 0.5);
	}

	int optimize_segments()
	{
		int c = 0;
		while(1)
		{
			bool did_opt = false;
			for(int i = 0; i < segments.size(); ++i)
			for(int j = 0; j < segments.size(); ++j)
			if(i != j)
			if(segments[i].can_optimize_with(segments[j]))
			{
				did_opt = true;
				segments[i].do_optimize_with(segments[j]);
				segments.erase(segments.begin() + j);
				++c;
//				fprintf(stderr, "optimized!\n");
				goto bail;
			}

			bail:
			if(!did_opt) break;	
		}
		return c;
	}
	
	void add_obj_left(const char * name, grading_type grad, float lat1, float lat2, float psi1, float psi2, float off1, float off2, float spa1, float spa2, float frq1, float frq2, perlin_params * noise)
	{
		obj_placement obj;
		obj.models.push_back(name);
		obj.grad = grad;
		obj.lat[0] = lat1; obj.lat[1] = lat2;
		obj.psi[0] = psi1; obj.psi[1] = psi2;
		obj.off[0] = off1; obj.off[1] = off2;
		obj.spa[0] = spa1; obj.spa[1] = spa2;
		obj.frq[0] = frq1; obj.frq[1] = frq2;
		if(noise)
			obj.wave = *noise;
		else
			obj.wave = def_perlin;
		objects.push_back(obj);		
	}
	
	void add_obj_right(const char * name, grading_type grad, float lat1, float lat2, float psi1, float psi2, float off1, float off2, float spa1, float spa2, float frq1, float frq2, perlin_params * noise)
	{
		float w = max_x();
		add_obj_left(name, grad, lat1 + w, lat2 + w, psi1, psi2, off1, off2, spa1, spa2, frq1, frq2, noise);
	}	

	void add_obj_left(const char ** name, grading_type grad, float lat1, float lat2, float psi1, float psi2, float off1, float off2, float spa1, float spa2, float frq1, float frq2, perlin_params * noise)
	{
		obj_placement obj;
		copy_string_vector(name,obj.models);
		obj.grad = grad;
		obj.lat[0] = lat1; obj.lat[1] = lat2;
		obj.psi[0] = psi1; obj.psi[1] = psi2;
		obj.off[0] = off1; obj.off[1] = off2;
		obj.spa[0] = spa1; obj.spa[1] = spa2;
		obj.frq[0] = frq1; obj.frq[1] = frq2;
		if(noise)
			obj.wave = *noise;
		else
			obj.wave = def_perlin;
		objects.push_back(obj);		
	}
	
	void add_obj_right(const char ** name, grading_type grad, float lat1, float lat2, float psi1, float psi2, float off1, float off2, float spa1, float spa2, float frq1, float frq2, perlin_params * noise)
	{
		float w = max_x();
		add_obj_left(name, grad, lat1 + w, lat2 + w, psi1, psi2, off1, off2, spa1, spa2, frq1, frq2, noise);
	}	
	
};

road	make_deck_graded(shader& shad, const lod_range& lod, const vert_props& len,
								float x1, float y1, float s1,
								float x2, float y2, float s2,
								surface_type surf)
{
	if(shad.os != 0)
		fprintf(stderr,"ERROR: graded deck uses wrong shader.\n");
	road ret;
	road_segment seg;
	seg.shad = &shad;
	seg.surf =surf;
	seg.grad = graded;
	seg.x[0] = x1;	seg.x[1] = x2;
	seg.y[0] = y1;	seg.y[1] = y2;
	seg.s[0] = s1;	seg.s[1] = s2;
	seg.lod = lod;
	seg.length = len;
	
	ret.segments.push_back(seg);
	return ret;
}

road	make_deck_draped(shader& shad, const lod_range& lod, const vert_props& len,
								float x1, float s1,
								float x2, float s2,
								surface_type surf)
{
	if(shad.os == 0)
		fprintf(stderr,"ERROR: draped deck uses wrong shader.\n");
	road ret;
	road_segment seg;
	seg.shad = &shad;
	seg.surf =surf;
	seg.grad = draped;
	seg.x[0] = x1;	seg.x[1] = x2;
	seg.y[0] = 0;	seg.y[1] = 0;
	seg.s[0] = s1;	seg.s[1] = s2;
	seg.lod = lod;
	seg.length = len;
	
	ret.segments.push_back(seg);
	return ret;
}

road make_obj(const char * name, grading_type grad, float lat1, float lat2, float psi1, float psi2, float off1, float off2, float spa1, float spa2, float frq1, float frq2, perlin_params * noise)
{
	road ret;
	ret.add_obj_left(name, grad, lat1, lat2, psi1, psi2, off1, off2, spa1, spa2, frq1, frq2, noise);
	return ret;
}


road	graded_from_draped(shader& shad, const road& r)
{
	road ret(r);
	for(vector<road_segment>::iterator s = ret.segments.begin(); s != ret.segments.end(); ++s)
	{
//		fprintf(stderr,"Making graded.\n");
		s->shad = &shad;
		s->grad = graded;
		s->y[0] = 0;
		s->y[1] = 0;
	}
	for(vector<obj_placement>::iterator o = ret.objects.begin(); o != ret.objects.end(); ++o)
	{
		o->grad = graded;
	}
	return ret;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

class	published_road {
public:

	road	guts;
	int		id;
	string	name;
	float	rgb[3];
	int		show_level;
	float	width;

	static vector<published_road *>	all;

	float length() {
		if(guts.segments.empty()) return 1.0f;
		float s = guts.segments[0].length.length;
		for(int n = 1; n < guts.segments.size(); ++n)
			s = max(s, guts.segments[n].length.length);
		return s;
	}

	float cut() {
		if(guts.segments.empty()) return 0.0f;
		float s = guts.segments[0].length.cut;
		for(int n = 1; n < guts.segments.size(); ++n)
			s = max(s, guts.segments[n].length.cut);
		return s;
	}

	float center_marker() { return guts.markers[0].x; }
	float get_left_semi() { return center_marker(); }
	float get_right_semi() { return width - center_marker(); }

	published_road(int i, const road& g) : id(i), guts(g) { all.push_back(this); width = guts.max_x(); if(guts.markers.size() != 1) fprintf(stderr,"ERROR: road %d has no center marker.\n", id); }
	
	void output_one(FILE * fi) {
		fprintf(fi,"# %s\n",name.c_str());
		fprintf(fi,"ROAD_TYPE\t%d %.3f %.1f %.1f %.1f %.1f\n", id, width, length(), rgb[0],rgb[1],rgb[2]);
		if(show_level) fprintf(fi,"SHOW_LEVEL\t%d\n", show_level);
		
		for(vector<road_center_marker>::iterator m = guts.markers.begin(); m != guts.markers.end(); ++m)
			fprintf(fi,"ROAD_CENTER\t%.2f\n",m->x);
		
		float c = cut();
		if(c)
			fprintf(fi,"REQUIRE_EVEN\t%.2f\n",c);
		
		for(vector<road_segment>::iterator s = guts.segments.begin(); s != guts.segments.end(); ++s)
		{
			set_shader_width(fi,s->shad);
			if(s->grad == draped)
				fprintf(fi,"SEGMENT_DRAPED\t% 2d\t% 3d % 5d\t%4.2f\t% 6.2f % 7.1f\t% 6.2f % 7.1f %s\n",
					s->shad->idx, (int) s->lod.near, (int) s->lod.far, length() / s->length.length, s->x[0],s->s[0],s->x[1], s->s[1], surf_name(s->surf));
			else
			if(s->grad == graded)
				fprintf(fi,"SEGMENT_GRADED\t% 2d\t% 3d % 5d\t%4.2f\t% 6.2f % 6.2f % 7.1f\t% 6.2f % 6.2f % 7.1f %s\n",
					s->shad->idx, (int) s->lod.near, (int) s->lod.far, length() / s->length.length, s->x[0],s->y[0], s->s[0],s->x[1], s->y[1], s->s[1], surf_name(s->surf));
		}
		
		for(vector<obj_placement>::iterator o = guts.objects.begin(); o != guts.objects.end(); ++o)
		{
			if(o->models.empty())	fprintf(stderr, "ERROR: an object has no models.\n");
			fprintf(fi,"OBJECT_RANDOM\t%30s %5.1f %5.1f  %5.1f %5.1f  %d  %5.1f %5.1f  %5.1f %5.1f\n",
				o->models.front().c_str(), o->lat[0], o->lat[1], o->psi[0], o->psi[1],
				o->grad == draped ? 1 : 0, o->off[0], o->off[1], o->spa[0], o->spa[1]);
			if(!o->wave.is_null())
				fprintf(fi,"OBJECT_FREQ\t%5.2f %5.2f   %5.1f %5.1f  %5.1f %5.1f  %5.1f %5.1f  %5.1f %5.1f\n",
					o->frq[0], o->frq[1],
					o->wave.w[0], o->wave.a[0],
					o->wave.w[1], o->wave.a[1],
					o->wave.w[2], o->wave.a[2],
					o->wave.w[3], o->wave.a[3]);
			for(int k = 1; k < o->models.size(); ++k)
				fprintf(fi,"OBJECT_ALT\t%s\n", o->models[k].c_str());
		}
		
		fprintf(fi,"\n");
	}
	
	static void output(FILE * fi) { for(vector<published_road*>::iterator r = all.begin(); r != all.end(); ++r) (*r)->output_one(fi); }

	void	widen(float new_width, float new_center)
	{
		if(new_width < width) fprintf(stderr,"WARNING: road id %d narrowed from %f to %f\n", id, width, new_width);
		if(new_center != center_marker())
			guts += (new_center - center_marker());
		width = new_width;
	}
	
	static published_road * find_by_id(int id)
	{
		for(vector<published_road*>::iterator r = all.begin(); r != all.end(); ++r)
		if((*r)->id == id)	return *r;
		return NULL;
	}
};

vector<published_road *> published_road::all;

void publish_road(int id, const string& name, int show_level, float r, float g, float b, const road& guts)
{
	published_road * rr = new published_road(id, guts);
	rr->rgb[0] = r;
	rr->rgb[1] = g;
	rr->rgb[2] = b;
	rr->name = name;
	rr->show_level = show_level;
	rr->guts.optimize_segments();
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

class	virtual_table {
public:
	virtual_table() { all.push_back(this); }
	
	string name;
	int	virtual_type;
	float max_pitch;
	float max_roll;
	vector<pair<float, int> > real_types;

	void output_one(FILE * fi) {
		published_road * pr = published_road::find_by_id(real_types.back().second);
		float real_w = pr ? pr->width : 0.0f;
		fprintf(fi,"# %s (%f)\n", name.c_str(), real_w);
		fprintf(fi,"ROAD_DRAPED\t%d %.2f %.2f\n", virtual_type, max_pitch, max_roll);
		for(int d = 0; d < real_types.size(); ++d)
			fprintf(fi,"ROAD_DRAPE_CHOICE\t % 5.1f %d\n",real_types[d].first, real_types[d].second);
		fprintf(fi,"\n");
	}

	void	sync_width() {
		float max_l = 0.0f, max_r = 0.0f;
		for(int n = 0; n < real_types.size(); ++n)
		if(real_types[n].second != 0)
		{
			published_road * pr = published_road::find_by_id(real_types[n].second);
			if(pr == NULL) fprintf(stderr,"ERROR: virtual type %d uses real type %d that is not defined.\n", virtual_type, real_types[n].second);
			
			max_l = max(max_l, pr->get_left_semi());
			max_r = max(max_r, pr->get_right_semi());
		}
		for(int n = 0; n < real_types.size(); ++n)
		if(real_types[n].second != 0)
		{
			published_road * pr = published_road::find_by_id(real_types[n].second);
			if(pr == NULL) fprintf(stderr,"ERROR: virtual type %d uses real type %d that is not defined.\n", virtual_type, real_types[n].second);
			pr->widen(max_l + max_r, max_l);
		}
	}
		

	static void output(FILE * fi) { for(vector<virtual_table*>::iterator v = all.begin(); v != all.end(); ++v) (*v)->output_one(fi); }
	
	static void sync_all_width() { for(vector<virtual_table*>::iterator v = all.begin(); v != all.end(); ++v) (*v)->sync_width(); }

	static vector<virtual_table*> all;

	
};

vector<virtual_table*> virtual_table::all;

void	make_draped_only(const string& name, int vt, float pitch)
{
	virtual_table * v = new virtual_table;
	v->virtual_type = vt;
	v->name = name;
	v->max_pitch = pitch;
	v->max_roll = 1.0;
	v->real_types.push_back(pair<float, int>(0.0, vt*100+1));
}

void	make_graded(const string& name, int vt, float pitch, float schedule[])
{
	virtual_table * v = new virtual_table;
	v->virtual_type = vt;
	v->max_pitch = pitch;
	v->max_roll = 0.0;
	v->name = name;
	
	int idx = 0;
	while(schedule[idx] || idx == 0)
	{
		if(idx == 0)
			v->real_types.push_back(pair<float,int>(schedule[idx], 0));
		else		
			v->real_types.push_back(pair<float,int>(schedule[idx], vt * 100 + idx + 1));
		++idx;
	}
}

void	make_draped(const string& name, int vt, float pitch, float roll, float schedule[])
{
	virtual_table * v = new virtual_table;
	v->virtual_type = vt;
	v->max_pitch = pitch;
	v->max_roll = roll;
	v->name = name;
	
	int idx = 0;
	while(schedule[idx] || idx == 0)
	{
		v->real_types.push_back(pair<float,int>(schedule[idx], vt * 100 + idx + 1));
		++idx;
	}
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

void output(FILE * fi)
{
	virtual_table::sync_all_width();
	fprintf(fi,"A\n800\nROADS\n\nONE_SIDED\n");
	shader::output(fi);
	published_road::output(fi);
	virtual_table::output(fi);
}

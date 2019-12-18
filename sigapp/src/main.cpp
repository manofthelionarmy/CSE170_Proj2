# include <sig/gs_event.h>
# include <sig/sn_lines.h>
# include <sig/sn_model.h>
# include <sig/sn_group.h>
# include <sig/sn_lines2.h>
# include <sig/sn_poly_editor.h>

# include <sigogl/ws_viewer.h>
# include <sigogl/ws_run.h>
#include<sig/sn_transform.h>




class ParametricCurveViewer;
class ScnViewer;

ParametricCurveViewer* PCV = NULL;
ScnViewer* SV = NULL;

/**
	Parametric Curve View Definition:
		- This will be our right hand window
*/
class ParametricCurveViewer : public WsViewer
{
public:
	// class member variables
	SnPolyEditor* _polyed;
	SnLines2* _curve;
	float begin;
	float end;
	// Parameter t
	float deltat; 
public:
	// Class member functions
	ParametricCurveViewer(SnNode* n, int x, int y, int w, int h);
	~ParametricCurveViewer() { ws_exit();  }
	//virtual void draw(GlRenderer* wr) override;

	void update_scene();
	virtual int handle(const GsEvent& e) override;
	virtual int handle_scene_event(const GsEvent& e) override;
};

/**
	Scene Viewer Definition:
		- This will be our left hand window and will hold our scene objects
*/
class ScnViewer : public WsViewer
{
public:
	// Class member variables; Add necesarry member variables to be used by this class here
public:
	// Class member functions; Add necessary functions here to be used by the class
	ScnViewer(SnNode* n, int x, int y, int w, int h);
	~ScnViewer() { ws_exit(); }
	virtual int handle(const GsEvent& e) override;
	virtual int handle_scene_event(const GsEvent& e) override;
	void build_scene();
};

/**
	An important callback function to handle adding new control points
*/

static void my_polyed_callback(SnPolyEditor* pe, enum SnPolyEditor::Event e, int pid) 
{
	ParametricCurveViewer* v = (ParametricCurveViewer*)pe->userdata();
	if (e == SnPolyEditor::PostMovement || e == SnPolyEditor::PostEdition || e == SnPolyEditor::PostInsertion)
	{
		v->update_scene();
	}
}

static float C(int n, int i) {
	float _n = (float)(gs_fact(n));
	float _i = (float)(gs_fact(i));
	float _n_i = (float)(gs_fact(n - i));
	return (_n) / (_i * _n_i);
}

// Go to office hours to check on my curves
GsPnt2 eval_bezier(float t, const GsArray<GsPnt2>& P)
{
	GsPnt2 point;

	// Calculating and will use in the bernstein polynomials
	float diff = 1.0f - t;
	int n = P.size() - 1;
	// Berstein polynomials
	float b0 = powf(diff, 3.0f);
	float b1 = 3 * t * powf(diff, 2.0f);
	float b2 = 3 * powf(t, 2.0f) * diff;
	float b3 = powf(t, 3.0f);

	float berstein_p;

	// point = (P[0] * b0) + (P[1] * b1) + (P[2] * b2) + (P.top() * b3);
	for (int i = 0; i < P.size(); ++i) {
		berstein_p = (float)(C(n, i)) * powf(t, (float)(i)) * powf(diff, (float)(n - i));
		point += (P[i] * berstein_p);
	}

	return point;
}

GsPnt2 crspline(float t, const GsArray <GsPnt2>& P)
{
	GsPnt2 point;

	int p = (int)t;

	GsPnt2 P1;
	GsPnt2 P2;

	GsPnt2 Ii;
	Ii = (P[p + 2] - P[p]) / 2.0f;

	P1 = P[p + 1] + (Ii / 3.0f);

	Ii = (P[p + 3] - P[p + 1]) / 2.0f;

	P2 = P[p + 2] - (Ii / 3.0f);

	GsArray<GsPnt2> c_p = GsArray<GsPnt2>(4);

	// (i+1)
	c_p[0] = P[p + 1];

	// (i+1)+
	c_p[1] = P1;

	// (i+2)-
	c_p[2] = P2;

	// (i+2)
	c_p[3] = P[p + 2];

	point = eval_bezier(t - p, c_p);

	return point;
}

ParametricCurveViewer::ParametricCurveViewer(SnNode* n, int x, int y, int w, int h) : WsViewer(x, y, w, h, "Parametric Curve View")
{
	if (!SV) gsout.fatal("Scence view has to be created first");

	cmd(WsViewer::VCmdAxis);

	view_all();

	rootg()->add(_polyed = new SnPolyEditor);
	rootg()->add(_curve = new SnLines2);

	_curve->color(GsColor::darkgreen);
	_curve->line_width(2.0f);

	// set initial control polygon:
	_polyed->callback(my_polyed_callback, this);
	_polyed->max_polygons(1);
	_polyed->solid_drawing(0);
	GsPolygon& P = _polyed->polygons()->push();
	P.setpoly("-2 -2  -1 1  1 0  2 -2");
	P.open(true);

	begin = 0.0f;
	end = 1.0f;
	deltat = 0.01f;

	update_scene();

	show();
}

void ParametricCurveViewer::update_scene()
{
	_curve->init();

	GsPolygon& P = _polyed->polygon(0);

	_curve->begin_polyline();

	end = (float)(P.size() - 3);

	for (float t = begin; t <= end; t += 0.01f)
	{
		_curve->push(crspline(t, P));
	}
	_curve->end_polyline();
}

int ParametricCurveViewer::handle(const GsEvent& e)
{
	if (e.type == GsEvent::Keyboard)
	{
		if (e.key == GsEvent::KeyEsc) ws_exit();
	}

	return WsViewer::handle(e);
}

int ParametricCurveViewer::handle_scene_event(const GsEvent& e)
{
	return WsViewer::handle_scene_event(e);
}

/**
	Scene Viewer
*/

ScnViewer::ScnViewer(SnNode* n, int x, int y, int w, int h) : WsViewer(x, y, w, h, "Scene View")
{

	cmd(WsViewer::VCmdAxis);

	view_all();
	build_scene();
	show();
}

int ScnViewer::handle(const GsEvent& e)
{
	if (e.type == GsEvent::Keyboard)
	{
		if (e.key == GsEvent::KeyEsc) ws_exit();

		if (e.key == GsEvent::KeySpace)
		{
			render();
			return 1;
		}
	}

	int ret = WsViewer::handle(e);

	return ret;
}

int ScnViewer::handle_scene_event(const GsEvent& e)
{
	return WsViewer::handle_scene_event(e);
}

void ScnViewer::build_scene() {
	SnGroup* g = new SnGroup; //to hold all the models and transformations
	SnModel* model[1];		//to hold all the models 
	SnTransform* t[1];		//to hold all the transformations 
	GsMat m[1];
	GsBox b0, b1, b2;
	


	for (int i = 0; i < 1; i++)
	{
		model[i] = new SnModel;
		t[i] = new SnTransform;
		//g->add(model[i]);		
	}


	if (!model[0]->model()->load("../Town/town2.obj"))
	{

		gsout << "poly.obj was not loaded" << gsnl;

	}

	//model[1]->model()->centralize();
	model[0]->model()->get_bounding_box(b0);
	t[0] = new SnTransform;
	m[0].translation(GsVec(0.0f, 0.0f, 0.0f));
	t[0]->set(m[0]);


	for (int i = 0; i < 1; i++)
	{
		g->add(t[i]);
		g->add(model[i]);
	}


	rootg()->add(g);

}

int main ( int argc, char** argv )
{
	/*MyViewer* v = new MyViewer ( -1, -1, 640, 480, "My SIG Application" );
	v->cmd ( WsViewer::VCmdAxis );

	v->view_all ();
	v->show ()*/;

	// define dimensions:
	int sw, sh, ww = 500, wh = 400, sep = 20;
	ws_screen_resolution(sw, sh);
	int x = (sw - sep - 2 * ww) / 2;

	SV = new ScnViewer(NULL, x, -1, ww, wh);

	PCV = new ParametricCurveViewer(NULL, x + ww + sep, -1, ww, wh);

	ws_run ();
}

# include <sig/gs_event.h>
# include <sig/sn_lines.h>
# include <sig/sn_model.h>
# include <sig/sn_group.h>
# include <sig/sn_lines2.h>
# include <sig/sn_poly_editor.h>
# include <sig/sn_manipulator.h>
# include <sig/sn_primitive.h>

# include <sigogl/ws_viewer.h>
# include <sigogl/ws_run.h>
# include <sig/sn_transform.h>




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
	SnLines* _curve;
	SnGroup* control_points;
	SnTransform* t_cp;
	float begin;
	float end;
	// Parameter t
	float deltat; 

	friend class ScnViewer;
public:
	// Class member functions
	ParametricCurveViewer(SnNode* n, int x, int y, int w, int h);
	~ParametricCurveViewer() { ws_exit();  }
	//virtual void draw(GlRenderer* wr) override;
	void add_model(SnGroup *parentg, SnShape* s, GsVec p);
	void add_edges();
	void draw_edges();
	GsPnt evalBezierUV(const GsArray<GsPnt>& cp, float u, float v);
	void draw_curves();
	void build_scene();
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


static GsPnt getPosition(const GsMat& m) {
	float x, y, z;
	x = m.e14;
	y = m.e24;
	z = m.e34;

	return GsPnt(x, y, z);
}

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

static void control_points_callback(SnManipulator* pe, const GsEvent& e, void *pid)
{
	ParametricCurveViewer* v = (ParametricCurveViewer*)pe->userdata();

	switch (e.type)
	{
		case GsEvent::Drag: {
			v->update_scene();
		}
	}
}

static void line_callback(SnManipulator* pe, const GsEvent& e, void* pid)
{
	ParametricCurveViewer* v = (ParametricCurveViewer*)pe->userdata();

	switch (e.button1) {
		case 1: {
			
			GsPnt clicked = e.mousep;

			GsArray<GsPnt> cp; 

			// need to copy all of the cp into an array
			SnGroup* g = v->rootg()->get<SnGroup>(0)->get<SnGroup>(1);

			// copy points
			for (int i = 0; i < g->size(); ++i) {
				GsMat& m = g->get<SnManipulator>(i)->mat();

				cp.push(getPosition(m));
			}

			// Remove edges; depend on our control points, so we need to re-add them later
			v->rootg()->remove(2);

			// Then remove the control points
			if (v->rootg()->get<SnGroup>(0)->remove(1) == 0) {

				v->control_points = new SnGroup;
			}

			// Insert the mouse click in to the array of our control points
			for (int i = 0; i + 1 < cp.size(); ++i) {
				
				GsPnt p0 = cp[i];
				GsPnt p1 = cp[i + 1];

				// if clicked is greater than p0 and clicked is less than p1
				if (GsVec::compare(clicked, p0) == 1 && GsVec::compare(clicked, p1) == -1)
				{
					GsPnt& n_p = cp.insert(i + 1, 1);
					
					n_p = clicked;

					break;
				}
			}


			// Use the array of our control points to insert new primitives
			SnPrimitive* p;

			
			
			for (int i = 0; i < cp.size(); ++i) {

				p = new SnPrimitive(GsPrimitive::Capsule, 0.15f, 0.15f, 0.01f);
				p->prim().material.diffuse = GsColor::blue;
				v->add_model(v->control_points, p, cp[i]);
			}

			// add the control points to our group
			v->rootg()->get<SnGroup>(0)->add(v->control_points);


			// add the edges
			v->add_edges();

			//update the scene

			v->update_scene();

			// v->render();
			
		}
	}
	
}

static float C(int n, int i) {
	float _n = (float)(gs_fact(n));
	float _i = (float)(gs_fact(i));
	float _n_i = (float)(gs_fact(n - i));
	return (_n) / (_i * _n_i);
}

// Go to office hours to check on my curves
GsPnt eval_bezier(float t, const GsArray<GsPnt>& P)
{
	GsPnt point;

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

GsPnt2 crspline(float t, const GsArray <GsPnt>& P)
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

	GsArray<GsPnt> c_p = GsArray<GsPnt>(4);

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

	begin = 0.0f;
	end = 1.0f;
	deltat = 0.01f;

	build_scene();
	add_edges();

	update_scene();
	show();
}

GsPnt ParametricCurveViewer::evalBezierUV(const GsArray<GsPnt>& cp, float u, float v)
{
	GsArray<GsPnt> c = GsArray<GsPnt>(1);


	// calculate new control point in the u
	c[0] = eval_bezier(u, cp);

	// calcualate new control point in the v
	return eval_bezier(v, c);
}

void ParametricCurveViewer::draw_curves()
{
	

	// Just extracted our control points group; The 2nd item in our group is our control points
	SnGroup* g = rootg()->get<SnGroup>(0)->get<SnGroup>(1);

	// The size of our control points is N-1 because our first element in the group is a transform
	GsArray<GsPnt> cp = GsArray<GsPnt>(g->size());

	for (int i = 0, j = 0; i < g->size(); ++i, ++j)
	{

		// Store the control points
		SnManipulator* p = g->get<SnManipulator>(i);
		GsMat& m = p->mat();
		cp[j] = getPosition(m);

	}

	_curve->init();

	_curve->begin_polyline();

	end = (float)(cp.size() - 3);


	// Goal, figure out a way how to do crspline with u,v 
	/*for (float t = begin; t <= end; t += 0.01f)
	{
		_curve->push((t, cp));
	}*/

	int N = 16;
	for (int i = 0; i <= N; ++i) {
		for (int j = 0; j <= N; ++j) {
			_curve->push( evalBezierUV( cp, i / float(N),  j / float(N)) );
		}
	}
	_curve->end_polyline();
}

void ParametricCurveViewer::add_edges()
{
	SnGroup* edges = new SnGroup;
	SnGroup* g; 
	SnLines* s;
	SnManipulator* manip;

	// the first group in root is the parent points. The second group in parent points is control points
	size_t n = rootg()->get<SnGroup>(0)->get<SnGroup>(1)->size();
	float w = 2.0f;

	for (int i = 0; i < n; ++i) {
		g = new SnGroup;
		s = new SnLines;

		// May have to add SnManipulator
		manip = new SnManipulator;
		
		// add a call back
		manip->callback(line_callback, this);
		manip->draw_box(false);
		s->line_width(w);
		s->color(GsColor::darkred);

		g->add(s);

		

		manip->child(g);
		edges->add(manip);
	}

	edges->separator(true);
	rootg()->add(edges);
}

void ParametricCurveViewer::draw_edges()
{
	// Prefetch the edges
	SnGroup* edges = rootg()->get<SnGroup>(2);
	
	// Prefetch the control points
	SnGroup* cp = rootg()->get<SnGroup>(0)->get<SnGroup>(1);

	size_t n = edges->size();

	SnLines* s;

	// j = 1 because the 0th element is our transform
	for (int i = 0, j = 0; i < n; ++i, ++j)
	{
		

		if (j + 1 < n) {

			// Get the positions of our control points

			GsMat& m1 = cp->get<SnManipulator>(j)->mat();
			GsMat& m2 = cp->get<SnManipulator>(j + 1)->mat();
			
			GsPnt p1 = getPosition(m1);
			GsPnt p2 = getPosition(m2);

			s = edges->get<SnManipulator>(i)->child<SnGroup>()->get<SnLines>(0);

			s->init();

			s->begin_polyline();

			s->push(p1);
			s->push(p2);

			s->end_polyline();


		}

	}
}

void ParametricCurveViewer::add_model(SnGroup* parentg, SnShape* s, GsVec p)
{
	SnManipulator* manip = new SnManipulator;
	GsMat m;
	m.translation(p);
	manip->initial_mat(m);
	manip->callback(control_points_callback, this);

	SnGroup* g = new SnGroup;

	SnLines* l = new SnLines;
	l->color(GsColor::green);

	g->add(s);
	g->add(l);

	manip->child(g);
	parentg->add(manip);
}

// We will Place our Control Polygons in here
void ParametricCurveViewer::build_scene()
{
	SnGroup* lines = new SnGroup;

	SnGroup* parent_points = new SnGroup;
	
	lines->add(_curve = new SnLines);

	_curve->color(GsColor::darkgreen);
	_curve->line_width(2.0f);

	control_points = new SnGroup;

	parent_points->add(t_cp = new SnTransform);
	parent_points->separator(true);

	SnPrimitive* p;

	/**
		First set of control points
	*/
	p = new SnPrimitive(GsPrimitive::Capsule, 0.15f, 0.15f, 0.01f);
	p->prim().material.diffuse = GsColor::blue;
	add_model(control_points, p, GsVec(-2, -2, 0));

	p = new SnPrimitive(GsPrimitive::Capsule, 0.15f, 0.15f, 0.01f);
	p->prim().material.diffuse = GsColor::blue;
	add_model(control_points, p, GsVec(-1, 0, 0));

	p = new SnPrimitive(GsPrimitive::Capsule, 0.15f, 0.15f, 0.01f);
	p->prim().material.diffuse = GsColor::blue;
	add_model(control_points, p, GsVec(1, 0, 0));

	p = new SnPrimitive(GsPrimitive::Capsule, 0.15f, 0.15f, 0.01f);
	p->prim().material.diffuse = GsColor::blue;
	add_model(control_points, p, GsVec(2, -2, 0));


	parent_points->add(control_points);
	// Append to the root; the first item at the root
	rootg()->add(parent_points);

	// our bezier curve; the second item at the root
	rootg()->add(lines);
}

void ParametricCurveViewer::update_scene()
{

	draw_edges();
	draw_curves();
	
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
		if (e.key == GsEvent::KeyEnter)
		{
			try {
				//PCV->_curve->init();
				PCV->_curve->color(GsColor::random());
				PCV->update_scene();
				PCV->render();
			}
			catch (...) {
				gsout << "didn't work" << gsnl;
			}
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

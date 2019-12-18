# include <sig/gs_event.h>
# include <sig/sn_lines.h>
# include <sig/sn_model.h>
# include <sig/sn_group.h>

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
public:
	// Class member functions
	ParametricCurveViewer(SnNode* n, int x, int y, int w, int h);
	~ParametricCurveViewer() { ws_exit();  }
	//virtual void draw(GlRenderer* wr) override;
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

ParametricCurveViewer::ParametricCurveViewer(SnNode* n, int x, int y, int w, int h) : WsViewer(x, y, w, h, "Parametric Curve View")
{
	if (!SV) gsout.fatal("Scence view has to be created first");

	cmd(WsViewer::VCmdAxis);

	view_all();

	show();
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

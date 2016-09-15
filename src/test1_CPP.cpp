
#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>
#include <CAM/CAMAll.h>

using namespace adsk::core;
using namespace adsk::fusion;
using namespace adsk::cam;

Ptr<Application> app;
Ptr<UserInterface> ui;

extern "C" XI_EXPORT bool run(const char* context)
{
	app = Application::get();
	ui = app->userInterface();
	Ptr<Design> design = app->activeProduct();

	// Get the root component of the active design.
	Ptr<Component> rootComp = design->rootComponent();

	// Create a new sketch on the active design.
	Ptr<Sketches> sketches = rootComp->sketches();
	Ptr<ConstructionPlane> xyPlane = rootComp->xYConstructionPlane();
	Ptr<Sketch> sketch = sketches->add(xyPlane);
	Ptr<SketchCurves> curves = sketch->sketchCurves();

	// Draw lines.
	Ptr<SketchLines> lines = curves->sketchLines();

	Ptr<SketchLine> line1 = lines->addByTwoPoints(Point3D::create(0, 0, 0), Point3D::create(3, 1, 0));
	Ptr<SketchLine> line2 = lines->addByTwoPoints(Point3D::create(4, 3, 0), Point3D::create(2, 4, 0));
	Ptr<SketchLine> line3 = lines->addByTwoPoints(Point3D::create(-1, 0, 0), Point3D::create(0, 4, 0));

	// Draw circles.
	Ptr<SketchCircles> circles = curves->sketchCircles();

	Ptr<SketchCircle> circle1 = circles->addByCenterRadius(adsk::core::Point3D::create(0,0,0), 2);
	Ptr<SketchCircle> circle2 = circles->addByCenterRadius(Point3D::create(8, 3, 0), 3);
	Ptr<SketchCircle> circle3 = circles->addByCenterRadius(circle2->centerSketchPoint(), 4);
	Ptr<SketchCircle> circle4 = circles->addByThreeTangents(line1, line2, line3, Point3D::create(0, 0, 0));

	// Apply tangent contstraints to maintain the relationship.
	Ptr<GeometricConstraints> constraints = sketch->geometricConstraints();

	Ptr<TangentConstraint> constraint1 = constraints->addTangent(circle4, line1);
	Ptr<TangentConstraint> constraint2 = constraints->addTangent(circle4, line2);
	Ptr<TangentConstraint> constraint3 = constraints->addTangent(circle4, line3);


	// Draw spline 1. Create an object collection for the points.
	Ptr<ObjectCollection> points = ObjectCollection::create();

	// Draw spline 2. Define the points the spline with fit through.
	points->add(Point3D::create(0, 0, 0));
	points->add(Point3D::create(5, 1, 0));
	points->add(Point3D::create(6, 4, 3));
	points->add(Point3D::create(7, 6, 6));
	points->add(Point3D::create(2, 3, 0));
	points->add(Point3D::create(0, 1, 0));

	// Draw spline 3. Create the spline.
	Ptr<SketchFittedSplines> splines = curves->sketchFittedSplines();
	splines->add(points);

	// Disp message on the messageBox
	ui->messageBox("Finish task.");

	return true;
}

extern "C" XI_EXPORT bool stop(const char* context)
{
	if (ui)
	{
		ui->messageBox("in stop");
		ui = nullptr;
	}

	return true;
}


#ifdef XI_WIN

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif // XI_WIN

#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <sstream>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace adsk::core;
using namespace adsk::fusion;

Ptr<Application> app;
Ptr<UserInterface> ui;
Ptr<Component> newComp;

namespace {

	// Create the command definition.
	Ptr<CommandDefinition> createCommandDefinition()
	{
		Ptr<CommandDefinitions> commandDefinitions = ui->commandDefinitions();
		if (!commandDefinitions)
			return nullptr;

		// Be fault tolerant in case the command is already added.
		Ptr<CommandDefinition> cmDef = commandDefinitions->itemById("SpurGear");
		if (!cmDef)
		{
			std::string resourcePath = "./resources";
			cmDef = commandDefinitions->addButtonDefinition("SpurGear",
				"Create Spur Gear",
				"Create a spur gear.",
				resourcePath);// absolute resource file path is specified
		}

		return cmDef;
	}

	// Calculate points along an involute curve.
	Ptr<Point3D> involutePoint(double baseCircleRadius, double distFromCenterToInvolutePoint)
	{
		double l = sqrt(distFromCenterToInvolutePoint * distFromCenterToInvolutePoint - baseCircleRadius * baseCircleRadius);
		double alpha = l / baseCircleRadius;
		double theta = alpha - acos(baseCircleRadius / distFromCenterToInvolutePoint);

		double x = distFromCenterToInvolutePoint * cos(theta);
		double y = distFromCenterToInvolutePoint * sin(theta);

		return Point3D::create(x, y, 0.0);
	}

	Ptr<ExtrudeFeature> createExtrude(Ptr<Profile> prof, double thickness)
	{
		if (!newComp)
			return nullptr;
		Ptr<Features> features = newComp->features();
		Ptr<ExtrudeFeatures> extrudes = features->extrudeFeatures();
		Ptr<ExtrudeFeatureInput> extInput = extrudes->createInput(prof, FeatureOperations::JoinFeatureOperation);
		Ptr<ValueInput> distance = ValueInput::createByReal(thickness);
		extInput->setDistanceExtent(false, distance);
		return extrudes->add(extInput);
	}

	Ptr<BRepFace> getCylinderFace(Ptr<Feature> feature)
	{
		Ptr<BRepFaces> faces = feature->faces();
		for (Ptr<BRepFace> face : faces)
		{
			if (face)
			{
				Ptr<Surface> geom = face->geometry();
				if (geom->surfaceType() == SurfaceTypes::CylinderSurfaceType)
					return face;
			}
		}

		return nullptr;
	}

	// Generate teeth of specific number
	void patternTeeth(Ptr<ObjectCollection> entities, Ptr<ExtrudeFeature> extrude, int numTeeth)
	{
		if (!newComp)
			return;
		Ptr<Features> features = newComp->features();
		Ptr<CircularPatternFeatures> circularPatterns = features->circularPatternFeatures();

		Ptr<BRepFace> face = getCylinderFace(extrude);

		Ptr<CircularPatternFeatureInput> circularInput = circularPatterns->createInput(entities, face);
		Ptr<ValueInput> quantity = ValueInput::createByReal((double)numTeeth);
		circularInput->quantity(quantity);
		circularPatterns->add(circularInput);
	}

	// Construct a gear.
	void buildGear(double diametralPitch, int numTeeth, double pressureAngle, double thickness)
	{
		// Create new component
		Ptr<Product> product = app->activeProduct();
		Ptr<Design> design = product;
		Ptr<Component> rootComp = design->rootComponent();
		Ptr<Occurrences> allOccs = rootComp->occurrences();
		Ptr<Occurrence> newOcc = allOccs->addNewComponent(Matrix3D::create());
		newComp = newOcc->component();

		// Create a new sketch.
		Ptr<Sketches> sketches = newComp->sketches();
		Ptr<ConstructionPlane> xyPlane = newComp->xYConstructionPlane();
		Ptr<Sketch> sketch = sketches->add(xyPlane);
		Ptr<SketchCurves> curves = sketch->sketchCurves();

		// Compute the various values for a gear.
		double pitchDia = (double)numTeeth / diametralPitch;
		double dedendum = 0.0;
		if (diametralPitch < (20 * (M_PI / 180)))
			dedendum = 1.157 / diametralPitch;
		else
			dedendum = 1.25 / diametralPitch;
		double rootDiameter = pitchDia - 2 * dedendum;
		double baseCircleDiameter = pitchDia * cos(pressureAngle);
		double outsideDia = (double)(numTeeth + 2) / diametralPitch;

		// Calculate points along the involute curve.
		int involutePointCount = 10;
		double involuteIntersectionRadius = baseCircleDiameter / 2.0;
		Ptr<ObjectCollection> involutePoints = ObjectCollection::create();
		double radiusStep = ((outsideDia - involuteIntersectionRadius * 2) / 2) / (involutePointCount - 1);
		for (int i = 0; i < involutePointCount; ++i)
		{
			Ptr<Point3D> newPoint = involutePoint(baseCircleDiameter / 2.0, involuteIntersectionRadius);
			involutePoints->add(newPoint);
			involuteIntersectionRadius = involuteIntersectionRadius + radiusStep;
		}

		// Determine the angle between the X axis and a line between the origin of the curve
		// and the intersection point between the involute and the pitch diameter circle.
		Ptr<Point3D> pitchInvolutePoint = involutePoint(baseCircleDiameter / 2.0, pitchDia / 2.0);
		double pitchPointAngle = atan(pitchInvolutePoint->y() / pitchInvolutePoint->x());

		// Determine the angle defined by the tooth thickness as measured at
		// the pitch diameter circle.
		double tooththicknessAngle = -(2 * M_PI) / (2 * numTeeth);

		// Rotate the involute so the intersection point lies on the x axis.
		double cosAngle = cos(-pitchPointAngle + (tooththicknessAngle / 2));
		double sinAngle = sin(-pitchPointAngle + (tooththicknessAngle / 2));
		for (Ptr<Point3D> involutePt : involutePoints)
		{
			involutePt->x(involutePt->x() * cosAngle - involutePt->y() * sinAngle);
			involutePt->y(involutePt->x() * sinAngle + involutePt->y() * cosAngle);
		}

		// Create a new set of points with a negated y.  This effectively mirrors the original
		// points about the X axis.
		Ptr<ObjectCollection> involute2Points = ObjectCollection::create();
		for (int i = 0; i < involutePointCount; ++i)
		{
			Ptr<Point3D> involutePt = involutePoints->item(i);
			Ptr<Point3D> newPt = Point3D::create(involutePt->x(), -involutePt->y(), 0.0);
			if (!newPt)
				return;
			involute2Points->add(newPt);
		}

		std::vector<double> curve1Dist, curve1Angle;
		for (Ptr<Point3D> involutePt : involutePoints)
		{
			curve1Dist.push_back(sqrt(involutePt->x() * involutePt->x() + involutePt->y() * involutePt->y()));
			curve1Angle.push_back(atan(involutePt->y() / involutePt->x()));
		}

		std::vector<double> curve2Dist, curve2Angle;
		for (Ptr<Point3D> involutePt : involute2Points)
		{
			curve2Dist.push_back(sqrt(involutePt->x() * involutePt->x() + involutePt->y() * involutePt->y()));
			curve2Angle.push_back(atan(involutePt->y() / involutePt->x()));
		}

		sketch->isComputeDeferred(true);
		double angleDiff = -tooththicknessAngle * 2;

		// Create the first spline.
		Ptr<SketchFittedSplines> splines = curves->sketchFittedSplines();
		Ptr<SketchFittedSpline> spline1 = splines->add(involutePoints);
		Ptr<SketchFittedSpline> spline2 = splines->add(involute2Points);

		double currentAngle = 0.0;

		Ptr<SketchLines> lines = curves->sketchLines();
		if (baseCircleDiameter >= rootDiameter)
		{
			Ptr<Point3D> rootPoint1 = Point3D::create((rootDiameter / 2) * cos(curve1Angle[0] + currentAngle), (rootDiameter / 2) * sin(curve1Angle[0] + currentAngle), 0.0);
			lines->addByTwoPoints(rootPoint1, spline1->startSketchPoint());

			Ptr<Point3D> rootPoint2 = Point3D::create((rootDiameter / 2) * cos(curve2Angle[0] + (currentAngle)), (rootDiameter / 2) * sin(curve2Angle[0] + (currentAngle)), 0);
			lines->addByTwoPoints(rootPoint2, spline2->startSketchPoint());
		}

		Ptr<Point3D> midPoint = Point3D::create((outsideDia / 2) * cos(currentAngle), (outsideDia / 2) * sin(currentAngle), 0.0);

		Ptr<SketchArcs> arcs = curves->sketchArcs();
		arcs->addByThreePoints(spline1->endSketchPoint(), midPoint, spline2->endSketchPoint());

		currentAngle = angleDiff;

		// Rotate the involute points for the next tooth.
		for (int i = 0; i < involutePointCount; ++i)
		{
			Ptr<Point3D> involutePt = involutePoints->item(i);
			involutePt->x(curve1Dist[i] * cos(curve1Angle[i] + currentAngle));
			involutePt->y(curve1Dist[i] * sin(curve1Angle[i] + currentAngle));

			Ptr<Point3D> involute2Pt = involute2Points->item(i);
			involute2Pt->x(curve2Dist[i] * cos(curve2Angle[i] + currentAngle));
			involute2Pt->y(curve2Dist[i] * sin(curve2Angle[i] + currentAngle));
		}

		Ptr<SketchCircles> circles = curves->sketchCircles();
		circles->addByCenterRadius(Point3D::create(0.0, 0.0, 0.0), rootDiameter / 2);

		sketch->isComputeDeferred(false);

		// Create the extrusion.
		Ptr<Profiles> profs = sketch->profiles();

		Ptr<Profile> profOne = profs->item(0);
		Ptr<ExtrudeFeature> extOne = createExtrude(profOne, thickness);

		Ptr<Profile> profTwo = profs->item(1);
		Ptr<ExtrudeFeature> extTwo = createExtrude(profTwo, thickness);

		// rotate copy tooth pattern
		Ptr<ObjectCollection> entities = ObjectCollection::create();
		entities->add(extTwo);
		patternTeeth(entities, extOne, numTeeth);

		// Rename the body
		Ptr<BRepFaces> faces = extOne->faces();
		Ptr<BRepFace> face = faces->item(0);
		Ptr<BRepBody> body = face->body();
		std::stringstream ss;
		ss << "Gear (" << pitchDia << " pitch dia.)";
		body->name(ss.str());
	}

	bool isPureNumber(std::string str)
	{
		for (char c : str)
		{
			if (c < '0' || c > '9')
				return false;
		}

		return true;
	}
}

// CommandExecuted event handler.
class OnExecuteEventHander : public CommandEventHandler
{
public:
	void notify(const Ptr<CommandEventArgs>& eventArgs) override
	{
		if (!app)
			return;

		Ptr<Product> product = app->activeProduct();
		Ptr<UnitsManager> unitsMgr = product->unitsManager();
		Ptr<Event> firingEvent = eventArgs->firingEvent();
		Ptr<Command> command = firingEvent->sender();
		Ptr<CommandInputs> inputs = command->commandInputs();

		// We need access to the inputs within a command during the execute.
		Ptr<ValueCommandInput> diaPitchInput = inputs->itemById("diaPitch");
		Ptr<ValueCommandInput> pressureAngleInput = inputs->itemById("pressureAngle");
		Ptr<StringValueCommandInput> numTeethInput = inputs->itemById("numTeeth");
		Ptr<ValueCommandInput> thicknessInput = inputs->itemById("thickness");

		double diaPitch = 7.62;
		double pressureAngle = 20.0 * (M_PI / 180);
		int numTeeth = 24;
		double thickness = 3.5;

		if (!diaPitchInput || !pressureAngleInput || !numTeethInput || !thicknessInput)
		{
			ui->messageBox("One of the inputs don't exist.");
		}
		else
		{
			diaPitch = unitsMgr->evaluateExpression(diaPitchInput->expression(), "cm");
			pressureAngle = unitsMgr->evaluateExpression(pressureAngleInput->expression(), "deg");
			thickness = unitsMgr->evaluateExpression(thicknessInput->expression(), "cm");

			std::string numTeethValue = numTeethInput->value();
			if (!numTeethValue.empty())
			{
				int num = atoi(numTeethValue.c_str());
				numTeeth = (num > 0) ? num : numTeeth;
			}
		}

		buildGear(diaPitch, numTeeth, pressureAngle, thickness);
	}
};

class OnValidateInputsHandler : public ValidateInputsEventHandler
{
public:
	void notify(const Ptr<ValidateInputsEventArgs>& eventArgs) override
	{
		Ptr<Event> firingEvent = eventArgs->firingEvent();

		Ptr<Command> command = firingEvent->sender();

		Ptr<CommandInputs> inputs = command->commandInputs();

		Ptr<ValueCommandInput> diaPitchInput = inputs->itemById("diaPitch");
		Ptr<ValueCommandInput> pressureAngleInput = inputs->itemById("pressureAngle");
		Ptr<StringValueCommandInput> numTeethInput = inputs->itemById("numTeeth");
		Ptr<ValueCommandInput> thicknessInput = inputs->itemById("thickness");

		if (!diaPitchInput || !pressureAngleInput || !numTeethInput || !thicknessInput)
			return;

		if (!app)
			return;
		Ptr<Product> product = app->activeProduct();
		Ptr<UnitsManager> unitsMgr = product->unitsManager();

		double diaPitch = unitsMgr->evaluateExpression(diaPitchInput->expression(), "cm");
		double pressureAngle = unitsMgr->evaluateExpression(pressureAngleInput->expression(), "deg");
		double thickness = unitsMgr->evaluateExpression(thicknessInput->expression(), "cm");
		int numTeeth = 0;
		std::string numTeethValue = numTeethInput->value();
		if (!numTeethValue.empty() && isPureNumber(numTeethValue))
		{
			numTeeth = atoi(numTeethValue.c_str());
		}

		if (numTeeth < 3 || diaPitch <= 0 || thickness <= 0 || pressureAngle < 0 || pressureAngle > M_PI * 30 / 180)
			eventArgs->areInputsValid(false);
		else
			eventArgs->areInputsValid(true);
	}
};

// CommandDestroyed event handler
class OnDestroyEventHandler : public CommandEventHandler
{
public:
	void notify(const Ptr<CommandEventArgs>& eventArgs) override
	{
		adsk::terminate();
	}
};

// CommandCreated event handler.
class OnCommandCreatedEventHandler : public CommandCreatedEventHandler
{
public:
	void notify(const Ptr<CommandCreatedEventArgs>& eventArgs) override
	{
		if (eventArgs)
		{
			Ptr<Command> cmd = eventArgs->command();
			if (cmd)
			{
				cmd->isRepeatable(false);

				// Connect to the CommandExecuted event.
				Ptr<CommandEvent> onExec = cmd->execute();
				bool isOk = onExec->add(&onExecuteHander_);

				Ptr<CommandEvent> onDestroy = cmd->destroy();
				isOk = onDestroy->add(&onDestroyHandler_);

				Ptr<ValidateInputsEvent> onValidateInputs = cmd->validateInputs();
				isOk = onValidateInputs->add(&onValidateInputsHandler_);

				// Define the inputs.
				Ptr<CommandInputs> inputs = cmd->commandInputs();

				Ptr<ValueInput> initialVal = ValueInput::createByReal(7.62);
				inputs->addValueInput("diaPitch", "Diametral Pitch.", "cm", initialVal);

				Ptr<ValueInput> initialVal2 = ValueInput::createByReal(20.0 * (M_PI / 180));
				inputs->addValueInput("pressureAngle", "Pressure Angle", "deg", initialVal2);

				inputs->addStringValueInput("numTeeth", "Number of Teeth", "24");

				Ptr<ValueInput> initialVal4 = ValueInput::createByReal(2.0);
				inputs->addValueInput("thickness", "Gear Thickness", "cm", initialVal4);
			}
		}
	}
private:
	OnExecuteEventHander onExecuteHander_;
	OnDestroyEventHandler onDestroyHandler_;
	OnValidateInputsHandler onValidateInputsHandler_;
} cmdCreated_;

extern "C" XI_EXPORT bool run(const char* context)
{
	app = Application::get();
	ui = app->userInterface();;

	Ptr<Product> product = app->activeProduct();
	Ptr<Design> design = product;

	Ptr<CommandDefinition> command = createCommandDefinition();

	Ptr<CommandCreatedEvent> commandCreatedEvent = command->commandCreated();
	commandCreatedEvent->add(&cmdCreated_);
	command->execute();

	// prevent this module from being terminate when the script returns, because we are waiting for event handlers to fire
	adsk::autoTerminate(false);

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

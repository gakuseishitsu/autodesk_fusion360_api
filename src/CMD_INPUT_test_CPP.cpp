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
			cmDef = commandDefinitions->addButtonDefinition("LighteningCylinder",
				"Create Lightening Cylinder",
				"Create a lightening cylinder.",
				resourcePath);// absolute resource file path is specified
		}

		return cmDef;
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


	Ptr<ExtrudeFeature> createExtrude(Ptr<Profile> prof, double thickness, bool cut)
	{
		if (!newComp)
			return nullptr;
		Ptr<Features> features = newComp->features();
		Ptr<ExtrudeFeatures> extrudes = features->extrudeFeatures();
		Ptr<ExtrudeFeatureInput> extInput;

		if(cut)
			extInput = extrudes->createInput(prof, FeatureOperations::CutFeatureOperation);
		else
			extInput = extrudes->createInput(prof, FeatureOperations::JoinFeatureOperation);

		Ptr<ValueInput> distance = ValueInput::createByReal(thickness);
		extInput->setDistanceExtent(false, distance);
		return extrudes->add(extInput);
	}

	// Construct a lightening Cylinder
	void buildLighteningCylinder(double innerDiameter, double outerDiameter, double thicknessY, double thicknessZ, int numSupport)
	{
		//ui->messageBox("hello");

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

		// Draw circles.
		Ptr<SketchCircles> circles = curves->sketchCircles();

		Ptr<SketchCircle> circle1 = circles->addByCenterRadius(Point3D::create(0, 0, 0), innerDiameter / 2.0);
		Ptr<SketchCircle> circle2 = circles->addByCenterRadius(Point3D::create(0, 0, 0), innerDiameter / 2.0 + thicknessY);
		Ptr<SketchCircle> circle3 = circles->addByCenterRadius(Point3D::create(0, 0, 0), outerDiameter / 2.0 - thicknessY);
		Ptr<SketchCircle> circle4 = circles->addByCenterRadius(Point3D::create(0, 0, 0), outerDiameter / 2.0);

		
		// Create the extrusion.
		Ptr<Profiles> profs = sketch->profiles();

		Ptr<Profile> profOne1 = profs->item(1);
		Ptr<ExtrudeFeature> extOne1 = createExtrude(profOne1, thicknessZ,0);

		Ptr<Profile> profOne2 = profs->item(3);
		Ptr<ExtrudeFeature> extOne2 = createExtrude(profOne2, thicknessZ,0);


		// Draw rectangule
		Ptr<Sketch> sketch2 = sketches->add(xyPlane);
		Ptr<SketchCurves> curves2 = sketch2->sketchCurves();

		double px1 = thicknessY / 2.0;
		double px2 = -1 * thicknessY / 2.0;
		double py1 = innerDiameter / 2.0 + 0.9*thicknessY;
		double py2 = outerDiameter / 2.0 - 0.9*thicknessY;

		Ptr<SketchLines> lines = curves2->sketchLines();
		Ptr<SketchLine> line1 = lines->addByTwoPoints(Point3D::create(px1, py1, 0), Point3D::create(px1, py2, 0));
		Ptr<SketchLine> line2 = lines->addByTwoPoints(Point3D::create(px1, py2, 0), Point3D::create(px2, py2, 0));
		Ptr<SketchLine> line3 = lines->addByTwoPoints(Point3D::create(px2, py2, 0), Point3D::create(px2, py1, 0));
		Ptr<SketchLine> line4 = lines->addByTwoPoints(Point3D::create(px2, py1, 0), Point3D::create(px1, py1, 0));

		// Create the extrusion
		Ptr<Profiles> profs2 = sketch2->profiles();
		Ptr<Profile> profOne3 = profs2->item(0);
		Ptr<ExtrudeFeature> extOne3 = createExtrude(profOne3, thicknessZ,0);

		// pattern copy of support material
		Ptr<ObjectCollection> entities = ObjectCollection::create();
		entities->add(extOne3);
		patternTeeth(entities, extOne1, numSupport);

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

		Ptr<ValueCommandInput> innerDiameterInput = inputs->itemById("innerDiameter");
		Ptr<ValueCommandInput> outerDiameterInput = inputs->itemById("outerDiameter");
		Ptr<ValueCommandInput> thicknessYInput = inputs->itemById("thicknessY");
		Ptr<ValueCommandInput> thicknessZInput = inputs->itemById("thicknessZ");
		Ptr<StringValueCommandInput> numSupportInput = inputs->itemById("numSupport");

		double innerDiameter = 10.0;
		double outerDiameter = 20.0;
		double thicknessY = 2.0;
		double thicknessZ = 2.0;
		int numSupport = 3;

		if (!innerDiameterInput || !outerDiameterInput || !thicknessYInput || !thicknessZInput || !numSupportInput)
		{
			ui->messageBox("One of the inputs don't exist.");
		}
		else
		{
			innerDiameter = unitsMgr->evaluateExpression(innerDiameterInput->expression(), "mm");
			outerDiameter = unitsMgr->evaluateExpression(outerDiameterInput->expression(), "mm");
			thicknessY = unitsMgr->evaluateExpression(thicknessYInput->expression(), "mm");
			thicknessZ = unitsMgr->evaluateExpression(thicknessZInput->expression(), "mm");

			std::string numSupportInputValue = numSupportInput->value();
			if (!numSupportInputValue.empty() && isPureNumber(numSupportInputValue))
			{
				numSupport = atoi(numSupportInputValue.c_str());
			}
		}

		buildLighteningCylinder(innerDiameter, outerDiameter, thicknessY, thicknessZ, numSupport);

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

		Ptr<ValueCommandInput> innerDiameterInput = inputs->itemById("innerDiameter");
		Ptr<ValueCommandInput> outerDiameterInput = inputs->itemById("outerDiameter");
		Ptr<ValueCommandInput> thicknessYInput = inputs->itemById("thicknessY");
		Ptr<ValueCommandInput> thicknessZInput = inputs->itemById("thicknessZ");
		Ptr<StringValueCommandInput> numSupportInput = inputs->itemById("numSupport");

		if (!innerDiameterInput || !outerDiameterInput || !thicknessYInput || !thicknessZInput || !numSupportInput)
			return;

		if (!app)
			return;
		Ptr<Product> product = app->activeProduct();
		Ptr<UnitsManager> unitsMgr = product->unitsManager();

		double innerDiameter = unitsMgr->evaluateExpression(innerDiameterInput->expression(), "mm");
		double outerDiameter = unitsMgr->evaluateExpression(outerDiameterInput->expression(), "mm");
		double thicknessY = unitsMgr->evaluateExpression(thicknessYInput->expression(), "mm");
		double thicknessZ = unitsMgr->evaluateExpression(thicknessZInput->expression(), "mm");
		int numSupport = 0;
		std::string numSupportInputValue = numSupportInput->value();
		if (!numSupportInputValue.empty() && isPureNumber(numSupportInputValue))
		{
			numSupport = atoi(numSupportInputValue.c_str());
		}

		if (innerDiameter <= 0 || outerDiameter <= 0 || thicknessY <= 0 || thicknessZ < 0 || numSupport < 2)
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

				Ptr<ValueInput> initialVal = ValueInput::createByReal(1.0);
				inputs->addValueInput("innerDiameter", "inner diameter.", "mm", initialVal);

				Ptr<ValueInput> initialVal2 = ValueInput::createByReal(2.0);
				inputs->addValueInput("outerDiameter", "outer diameter.", "mm", initialVal2);

				Ptr<ValueInput> initialVal3 = ValueInput::createByReal(0.2);
				inputs->addValueInput("thicknessY", "Thickness of Support Material", "mm", initialVal3);

				Ptr<ValueInput> initialVal4 = ValueInput::createByReal(0.2);
				inputs->addValueInput("thicknessZ", "Part Thickness", "mm", initialVal4);

				inputs->addStringValueInput("numSupport", "Number of Support Material", "3");

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
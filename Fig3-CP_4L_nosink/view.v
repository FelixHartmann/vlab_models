[Main]
Seed: 975318557 // 975318557 // 276210057 // 140173803
CellShape: truncated_octahedron // cube
CellSize: 1 1 1 //0.97 //0.97 //1.07 //1.5 1.5 1  //cube: 1 1 1
GridSize: 7 1 4 // 9 1 2 // 9 1 1 // 8 1 4 // 14 1 1 //12 12 3 //10 10 3 //9 9 3 //11 11 3 //7 7 7
GridNoise: .12
NbSinks: 0 // 3
SinkPosition: 0.5 // 0.6  // 0: center  1: border  0.5: half radius
L1BorderSize: 3
CentralZoneProportion: .15
InterApoplastArea: 0.1
ApoplastWidth: 0.1
MinMembraneArea: 5e-2 // 5e-2
MinCellCellPolarisation: 0.005
MinCvFactor: 0.2
MaxNbCvCells: 50
DrawDt: 0.2
EndTime: 20000
PrintInterval: 6
StoppingThreshold: 5.0e-3
MaxTime: 100
MinVeinPolarisation: 6
MinVeinPolarisationVariation: 1e-2
InitScaling: 1 1 1

[Output]
DescVariablesFile: desc_variables.csv
MaxdPINVariablesFile: max_dPIN_variables.csv

[CellChemicals]
ApoplastAuxinDiffusion: .01
MembranePINDiffusion: 0.05
InitialAuxinConcentration: 0
AUXLAXConcentration: 0.3 // 1
PINTotalConcentration: 6
InitialPINCorpus: 3
InitialMembranePINConcentration: 0.01
PINBaseProduction: 0
AuxinDepPINProduction: .5 // 1 // 0.5
PINProductionSaturation: 1
PINTurnover: .05 // 0.005 // 0.05
AAUXFormation: 1 // 1
APINFormation: 1 // 1
InfluxByAAUX: 1 // 1
EffluxByAPIN: 2 // 2
AuxinProduction: 0
AuxinTurnover: .005
PINExocytosisConstitutive: 0.05
PINExocytosisAPIN: 4
PINExocytosisAAUX: 0.05
PINMembraneSaturation: 0
PINEndocytosisConstitutive: 2
APINBreakupLow: .3
APINBreakupHigh: 15
APINBreakupSlope: 25
AuxinThreshold: 1.65
VAFBinding: 0.1
VAFUnbinding: 0.1
VAFTurnover: 0.01
VAFDiffusion: 0.05 // 0.1 // 0.2
ExpBasePINRelocation: 3

[ChemicalsL1]
AuxinProductionL1: .1
PINBaseProductionL1: .01 // .1 // 0.01
AuxinDepPINProductionL1: .5 // 1 // 0.5
AUXLAXConcentrationL1: 0.3 // 1
InitialAuxinConcentrationL1: 1.3

[ChemicalsSink]
AuxinTurnoverSink: 5
VAFSurfaceProduction: 0.1 // 0.15 // 0.03 // 0.015 // 0.15

[View]
MaxViewAuxin: 4 // 4 // 12 // 6
MaxViewPIN: 2 // 2
MaxViewVAF: 0.5
ViewDt: 0.2

ColorL1Begin: 1
ColorL1End: 2
ColorCorpusBegin: 5
ColorCorpusEnd: 6
ColorPINBegin: 7
ColorPINEnd: 8
CellSizeProportion: 0.8
PINSize: 0.1
EdgeSize: 0.3
SinkColor: 15
TransparentCells: true
TransparentPIN: true

ShowVertices: false
ShowEdges: false
ShowFaces: false
ShowCells: true
ShowFaceNormals: false
ShowVertexNormals: false
ShowBorder: false
ShowCompression: false // red = extension, blue = compression

NormalSize: 0.2

CellShift: 0.05
CellCenterSize: 0.1

[ComplexDrawer]
CellShift: 0.15
CellCenterSize: 0.1
VertexColor: 6
EdgeColor: 6
FaceNormalColor: 6
NormalSize: 0.4

[SolverGraphDrawer]
PlotSolverGraph: false
ColorCell: 16
ColorMembrane: 17
ColorApoplast: 18
ColorLink: 19
LinkThickness: 2
SphereSize: 0.07

[Solver]
Solver: AdaptiveCrankNicholson  // Euler, AdaptiveEuler, Fixedpoint,
                                        // Midpoint, RungeKutta,
                                        // AdaptiveRungeKutta,
                                        // AdaptiveCrankNicholson

// Global help:
//  *TolType can be MeanComponent or MaxComponent

// Various Timesteps
EulerDt: .002//.01 //.0025			// Timestep for Euler solver
FixedPointDt: .01			// Timestep for fixed point solver
MidPointDt: .06				// Timestep for midpoint solver
RungeKuttaDt: .01			// Timestep for Runge-Kutta 4th order
InitialDt: 0.01				// Beginning timestep for adaptive solvers
MaxDt: 0.5				// Maximum Dt

// Adaptive Euler parms
AEulerIncDt: .1				// Adaptive Euler Dt increment/decrement
AEulerResDt: .5				// Adaptive Euler restart Dt decrement
AEulerMinDt: .001			// Adaptive Euler min Dt
AEulerMaxDt: .25			// Adaptive Euler max Dt
AEulerTolType: MaxComponent		// Tolerence type for adaptive Euler
AEulerResTol: 1				// Adaptive Euler restart tolerance
AEulerLowTol: .3			// Adaptive Euler low water mark
AEulerHighTol: .4			// Adaptive Euler high water mark

// Fixed Point iteration parms
FixedPointMaxSteps: 5		// Max steps for fixed point iteration
FixedPointTol: .0001			// Tolerance for fixed point iteration
FixedPointTolType: MaxComponent

// Adaptive Runge-Kutta parms
ARungeIncDt: .01			// Adaptive Runge-Kutta Dt increment/decrement
ARungeResDt: .5				// Adaptive Runge-Kutta restart Dt decrement
ARungeMinDt: .001			// Adaptive Runge-Kutta min Dt
ARungeMaxDt: 1				// Adaptive Runge-Kutta max Dt
ARungeTolType: MaxComponent		// Tolerence type for adaptive Runge-Kutta
ARungeResTol: 1				// Adaptive Runge-Kutta restart tolerance
ARungeLowTol: .03			// Adaptive Runge-Kutta low water mark
ARungeHighTol: .05			// Adaptive Runge-Kutta high water mark

// Crank Nicholson params
CRIncDt: .2				// Increment for timestep based on efficiency (mulitple of current size)
CRResDt: .5				// Decrement for timestep if unsucc
CRAvgCPU: .5            // Weight of current dt in eff average
CRMinCPU: 5				// Minimun CPU, below this alway increases timestep

// Newton Tolerance (used by Crank Nicholson)
NewtTol: .00001				// Tolerence for Newton's method
NewtTolType: MaxComponent
NewtMaxSteps: 10 			// Max steps for Newton's method

// Conjugate gradient params (used by Crank Nicholson)
ConjGradTol: .00001			// Tolerence for conjugate gradient
ConjGradTolType: MaxComponent
ConjGradMaxSteps: .1			// Max steps for conjugate gradient (multiple of N)
ConstNbPartials: false			// Set if partials to neighbors are constant (diffusion only)

// Misc general parms
Dx: 1e-6				// Delta for numerical diff
PrintMatrix: false			// Print Matrix (Conj-Grad)
PrintStats: 0


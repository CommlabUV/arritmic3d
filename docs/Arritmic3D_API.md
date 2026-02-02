# Arritmic3D Python API

## `enum identifiers for SystemEventType`

> NODE_EVENT : Activation or deactivation in a node

> EXT_ACTIVATION : External activation of the tissue

> FILE_WRITE : Event for writing output to a file

> OTHER : Other events not defined

> NO_EVENT : There are no more events. Simulation has finished.

## `arritmic.CardiacTissue(ncells_x, ncells_y, ncells_z, x_spacing, y_spacing, z_spacing)`

Constructor of the CardiacTissue class.

**Parameters:**

> ncells_x, ncells_y, ncells_z: int
>
> x_spacing, y_spacing, z_spacing: float

## `InitPy(cell_types_, parameters_, fiber_orientation_)`

Initialize the tissue from Python. Calls Init with a vector of parameters.
It can be called again to initialize the data for a new simulation.

**Parameters:**

> cell_types_ : Vector of cell types.

> parameters_ : Dictionary with the parameters.

> fiber_orientation_ : Vector of fiber orientations.

## `InitModels(fileAPD, fileCV)`

Initialize the models for the APD and CV update. ** Should be called before InitPy **

> fileAPD : CSV file definition for APD models

> fileCV : CSV file definition for CV models

## `ChangeParameters(parameters_)`
Change the parameters of the tissue nodes. Simulation can continue normally.

## `size()`

Get the number of nodes in the tissue

## `GetNumLiveNodes()`

Get the number of nodes not VOID in the tissue

## `GetSizeX()`

Get the number of nodes in the X direction

## `GetSizeY()`

Get the number of nodes in the Y direction

## `GetSizeZ()`

Get the number of nodes in the Z direction

## `GetIndex (int  x, int  y, int  z)`

Get the id (index) of node with coordinates (x, y, z)

## `SetTimer (float  t)`

Set a timer for the simulation

**Parameters:**

> t : Period (time between events) in milliseconds.

## `GetTime()`

Get the current time of the tissue

## `GetStates()`

Get the states of the tissue nodes.

**Returns:**

> Vector of states (int) of the tissue nodes.

## `GetAPD()`

Get the APD of the tissue nodes.

**Returns:**

> Vector of APD of the tissue nodes.

## `GetCV()`

Get the conduction velocity of the tissue nodes.

**Returns:**

> Vector of conduction velocity of the tissue nodes.

## `GetDI()`

Get the DI (diastolic interval) of the tissue nodes.

**Returns:**

> Vector of DI of the tissue nodes.

## `GetLastDI()`

Get the DI (diastolic interval) of the last activation of the tissue nodes.

**Returns:**

> Vector of DI of the tissue nodes.

## `GetLAT()`

Get the LAT (Last Activation Time) of the tissue nodes.

**Returns:**

> Vector of LAT of the tissue nodes.

## `GetBeat()`

Get the beat id of the tissue nodes.

**Returns:**

> Vector of beat id (int) of the tissue nodes.

## `GetAPDVariation()`

Get the variation in the APD from last activation to actual activation of the tissue nodes.

**Returns:**

> Vector of APD variation of the tissue nodes.

## `GetAPDMeanVariation()`

Get the mean variation in the APD from last activation to actual activation of the tissue nodes.

**Returns:**

> Mean of APD variation (float)

## `ResetVariations()`

Reset the accumulator of the mean variation in the APD before the beginning of a new activation in the tissue.

## `GetSensorInfo()`

Get the information of all sensors.

**Returns:**

> A map where the key is the node ID and the value is a vector of sensor data.

## `GetSensorDataNames()`

Get the names of the data stored in the sensors.

**Returns:**

> A vector of strings containing the names of the data.

## `GetDefaultParameters()`

Return a dictionary with the default parameters for the nodes. int values are converted to float.

**Returns:**

> Dictionary with the parameters.

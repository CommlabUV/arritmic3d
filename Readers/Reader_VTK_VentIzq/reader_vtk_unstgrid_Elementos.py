import vtk, sys
import numpy as np
import math
import os 
from vtk.util.numpy_support import vtk_to_numpy

#### Script preparado para modelos orientados en el eje z (0,0,1)

def vecinos(cellId):
    cellPointIds = vtk.vtkIdList()
    dataVent.GetCellPoints(cellId, cellPointIds)
    neighbors = vtk.vtkIdList()
    for i in range (cellPointIds.GetNumberOfIds()):
        idList = vtk.vtkIdList()
        idList.InsertNextId(cellPointIds.GetId(i))

        neighborCellIds = vtk.vtkIdList()
        dataVent.GetCellNeighbors(cellId,idList, neighborCellIds)

        for j in range (neighborCellIds.GetNumberOfIds()):
            neighbors.InsertNextId(neighborCellIds.GetId(j))

    cellsneigh=np.zeros(neighbors.GetNumberOfIds())
    for i in range (neighbors.GetNumberOfIds()):
        cellsneigh [i]= neighbors.GetId(i)

    vecinos = np.unique(cellsneigh).astype(int)
    return vecinos.astype(int)

# Convierte Vector de orientación de fibras, de coordenadas globales a locales
def global2localCoord (FO, epi_N, endo_N, w, vectorLongAxes, i):
    
    # Normal of the current point -> normal direction of the local coordinates system
    normalDir = np.zeros(3)
    normalDir = w * epi_N - (1-w) * endo_N     #  epiNORMAL  -> OUT (pointing to the pericardium)
    normDir = np.linalg.norm(normalDir)
    normalDir = normalDir/normDir               #  endoNORMAL -> IN  (pointing to the blood pool) 
    # Tangent direction of the local coordinates system
    tangentDir = np.cross(vectorLongAxes, normalDir)
    tangentNormDir = np.linalg.norm(tangentDir)
    tangentDir = tangentDir/tangentNormDir
    # Axial direction of the local coordinates system
    axialDir = np.cross(normalDir, tangentDir)
    axialNormDir = np.linalg.norm(axialDir)
    axialDir = axialDir/axialNormDir

    # Create the transformation matrix
    M = np.matrix([normalDir, tangentDir, axialDir])
    M = M.transpose()
    FO = np.dot(M, FO)

    return FO

# Nombre archivos vtk
source = 'VTK_Clara_Footprint_OK/p10'
filenameCore = f'{source}/Core Surface.vtk'
filenameBZ = f'{source}/Border Zone Surface.vtk'
filenameEpi = f'{source}/Epi Layer.vtk'
filenameEndo = f'{source}/Endo Layer.vtk'

#### Voxelizamos archivo de resonancia #####
# Archivo de resonancia y malla ventrículo Izq
file_nameRes = f'{source}/Study2.vtk'
file_nameVent = f'{source}/Left Ventricle.vtk'
print("  Leídos .........................")
# Cargamos los datos de la mesh del ventrículo 
mesh_readerVent = vtk.vtkPolyDataReader()
mesh_readerVent.SetFileName(file_nameVent)
mesh_readerVent.Update()
mesh_polydataVent = mesh_readerVent.GetOutput()

# Cargamos los datos de la resonancia.
reader = vtk.vtkStructuredPointsReader()
reader.SetFileName(file_nameRes)
reader.Update() 

imageDim = reader.GetOutput().GetDimensions() 
boundsImage = reader.GetOutput().GetBounds()
cell = reader.GetOutput().GetCell(1)
cellSize = cell.GetBounds()
xsizeCell = cellSize[1]-cellSize[0]
ysizeCell = cellSize[3]-cellSize[2]
zsizeCell = cellSize[5]-cellSize[4]

print("Dim Cell Res: ", cellSize)
print("Dim Cell Res X Y Z: ", xsizeCell, ysizeCell, zsizeCell)
print("Bound Res: ", boundsImage)
print("Dim Res: ", imageDim)


# Cogemos bounds del ventrículo para reducir la resonancia a dicho cubo
boundsVent = mesh_polydataVent.GetBounds()
print("Bounds Vent: ", boundsVent)
# Extraemos dataset de la resonancia en tamaño ventrículo bounds
extractVOI = vtk.vtkExtractVOI()
extractVOI.SetInputData(reader.GetOutput())
# Definimos VOI alrededor del ventrículo dándole 5 elementos de más en x e y, el alto lo dejamos igual porque está ajustado al ventrículo
extractVOI.SetVOI(int(abs(boundsVent[0]-boundsImage[0])/xsizeCell)-5,int(abs(boundsVent[1]-boundsImage[0])/xsizeCell)+5,int(abs(boundsVent[2]-boundsImage[2])/ysizeCell)-5,int(abs(boundsVent[3]-boundsImage[2])/ysizeCell)+5, 0, int(imageDim[2]-1))
extractVOI.Update()


# Función implícita para evaluar las celdas que pertenecen al ventrículo
implicitPolyDataDistanceVent = vtk.vtkImplicitPolyDataDistance()
implicitPolyDataDistanceVent.SetInput(mesh_polydataVent)

# Extraemos solo gemometría del ventrículo
# Con este método no se cortan celdas, salen enteras (voxels)
# En script prova_reader tenemos los distintos métodos para remallar el voxelizado
# Con este método no se cortan celdas, salen enteras (voxels)
ugrid = vtk.vtkExtractGeometry()
ugrid.SetInputData(extractVOI.GetOutput())
ugrid.SetImplicitFunction(implicitPolyDataDistanceVent)
ugrid.ExtractInsideOn()
ugrid.ExtractBoundaryCellsOn()

ugrid.Update()
print("  Fin clip .........................")

# Creamos array para guardar la información de los valores scalars
scalarsVent = vtk.vtkFloatArray()
scalarsVent.SetNumberOfComponents(1)
scalarsVent.SetName("scalars")

# Pasamos PointData to CellData
for i in range(ugrid.GetOutput().GetNumberOfCells()):
    cell = ugrid.GetOutput().GetCell(i)
    pointsIDs = cell.GetPointIds()
    scalarElemAcum = 0 
    countPoint = 0
    for i in range(pointsIDs.GetNumberOfIds()):
        countPoint += 1
        pointId = pointsIDs.GetId(i)
        scalarElemAcum += ugrid.GetOutput().GetPointData().GetArray('scalars').GetValue(pointId)
    scalarsVent.InsertNextValue(scalarElemAcum / countPoint) 

ugrid.GetOutput().GetCellData().AddArray(scalarsVent)
ugrid.Update()
print("  Fin scalar CellData.........................")


#Cargamos los datos del ventriculo voxelizado
dataVent = ugrid.GetOutput()

# Obtenemos todas las propiedades que contiene el archivo vtk a nivel elemento y modo
print('\nPROPIEDADES ASIGNADAS AL ELEMENTO\n')
for i in range(dataVent.GetCellData().GetNumberOfArrays()):
    print (dataVent.GetCellData().GetArrayName(i)+'\n')

print('\nPROPIEDADES ASIGNADAS AL NODO\n')
for i in range(dataVent.GetPointData().GetNumberOfArrays()):
    print (dataVent.GetPointData().GetArrayName(i)+'\n')



# Cargamos los datos de la mesh del core 
mesh_readerCore = vtk.vtkPolyDataReader()
mesh_readerCore.SetFileName(filenameCore)
mesh_readerCore.Update()
mesh_polydataCore = mesh_readerCore.GetOutput()

# Cargamos los datos de la mesh del Border zone 
mesh_readerBZ = vtk.vtkPolyDataReader()
mesh_readerBZ.SetFileName(filenameBZ)
mesh_readerBZ.Update()
mesh_polydataBZ = mesh_readerBZ.GetOutput()

# Cargamos los datos de la Epi mesh 
# y subdividimos para aumentar número de puntos resolución 
mesh_readerEpi = vtk.vtkPolyDataReader()
mesh_readerEpi.SetFileName(filenameEpi)
mesh_readerEpi.Update()
cleanPolyDataEpi = vtk.vtkCleanPolyData()
cleanPolyDataEpi.SetInputData(mesh_readerEpi.GetOutput())
cleanPolyDataEpi.Update()
mesh_polydataEpiPre = cleanPolyDataEpi.GetOutput()

mesh_polydataEpiPost = vtk.vtkLinearSubdivisionFilter()
mesh_polydataEpiPost.SetInputData(mesh_polydataEpiPre)
mesh_polydataEpiPost.SetNumberOfSubdivisions(3)
mesh_polydataEpiPost.Update()
mesh_polydataEpi = mesh_polydataEpiPost.GetOutput()

# Cargamos los datos de la Endo mesh 
mesh_readerEndo = vtk.vtkPolyDataReader()
mesh_readerEndo.SetFileName(filenameEndo)
mesh_readerEndo.Update()
cleanPolyDataEndo = vtk.vtkCleanPolyData()
cleanPolyDataEndo.SetInputData(mesh_readerEndo.GetOutput())
cleanPolyDataEndo.Update()
mesh_polydataEndoPre = cleanPolyDataEndo.GetOutput()

mesh_polydataEndoPost = vtk.vtkLinearSubdivisionFilter()
mesh_polydataEndoPost.SetInputData(mesh_polydataEndoPre)
mesh_polydataEndoPost.SetNumberOfSubdivisions(3)
mesh_polydataEndoPost.Update()
mesh_polydataEndo = mesh_polydataEndoPost.GetOutput()

#Creamos 3 arrays distintos para coord centroide celda x, y, z. 
#Y array bound que nos devuelve de cada celda xMin, xMax, yMin, yMax, zMin, zMax, 
x = np.zeros(dataVent.GetNumberOfCells())
y = np.zeros(dataVent.GetNumberOfCells())
z = np.zeros(dataVent.GetNumberOfCells())
bounds = np.zeros(6)

# Creamos 3 arrays distintos para orientación de fibras
fibreOrientationX = np.zeros(dataVent.GetNumberOfCells())
fibreOrientationY = np.zeros(dataVent.GetNumberOfCells())
fibreOrientationZ = np.zeros(dataVent.GetNumberOfCells())

#Calculamos el centroide de cada celda y guardamos las coordenadas por separado
for i in range(dataVent.GetNumberOfCells()):
    bounds =  dataVent.GetCell(i).GetBounds()
    x[i] =  bounds[0]+((bounds[1]-bounds[0])/2)
    y[i] =  bounds[2]+((bounds[3]-bounds[2])/2)
    z[i] =  bounds[4]+((bounds[5]-bounds[4])/2)

#Creamos un array de los valores de intensidad de cada celda en el mismo orden que las coordenadas guardadas
scalar = np.zeros(dataVent.GetNumberOfCells())
for i in range(dataVent.GetNumberOfCells()):
    scalar[i]=dataVent.GetCellData().GetArray('scalars').GetValue(i)


# Función implícita para evaluar las celdas que pertenecen a la mesh del core
implicitPolyDataDistanceCore = vtk.vtkImplicitPolyDataDistance()
implicitPolyDataDistanceCore.SetInput(mesh_polydataCore)
# Función implícita para evaluar las celdas que pertenecen a la mesh del BZ
implicitPolyDataDistanceBZ = vtk.vtkImplicitPolyDataDistance()
implicitPolyDataDistanceBZ.SetInput(mesh_polydataBZ)
# Función implícita para evaluar la distancia de cada celda a Epi
implicitPolyDataDistanceEpi = vtk.vtkImplicitPolyDataDistance()
implicitPolyDataDistanceEpi.SetInput(mesh_polydataEpi)
# Función implícita para evaluar la distancia de cada celda a Endo
implicitPolyDataDistanceEndo = vtk.vtkImplicitPolyDataDistance()
implicitPolyDataDistanceEndo.SetInput(mesh_polydataEndo)
# Función implícita para localizar el punto mas cercano en Epi y calcular su normal 
implicitPolyDataDistanceEpiNorm = vtk.vtkImplicitPolyDataDistance()
implicitPolyDataDistanceEpiNorm.SetInput(mesh_polydataEpi)
# Función implícita para localizar el punto mas cercano en Endo y calcular su normal 
implicitPolyDataDistanceEndoNorm = vtk.vtkImplicitPolyDataDistance()
implicitPolyDataDistanceEndoNorm.SetInput(mesh_polydataEndo)

# Creamos array para guardar la información de las distancias  a Endo y EPi 
# para que al calcular las normales lcoalizar si algún punto está fuera del polydata e invertir 
# el cálculo del vector normal para que esté en al dirección correcta
distancesEndo = vtk.vtkFloatArray()
distancesEndo.SetNumberOfComponents(1)
distancesEndo.SetName("DistEndo")
distancesEpi = vtk.vtkFloatArray()
distancesEpi.SetNumberOfComponents(1)
distancesEpi.SetName("DistEpi")

# Creamos array para guardar la información de las distancias  Endo to EPi de 0 a 1
# Esto no hace falta si solo guardamos en numpy array para volcar a archivo txt
distancesEndo2Epi = vtk.vtkFloatArray()
distancesEndo2Epi.SetNumberOfComponents(1)
distancesEndo2Epi.SetName("DistEndoToEpi")

#Creamos un array de los valores de tipo de célula (scar 2, BZ 1, sana 0) en el mismo orden que las coordenadas guardadas
cellType = np.zeros(dataVent.GetNumberOfCells())
#Creamos un array de los valores de tipo de célula Endo, M, Epi (epi 2, Mid 1, endo 0) 
Endo2Epi = vtk.vtkFloatArray()
Endo2Epi.SetNumberOfComponents(1)
Endo2Epi.SetName("EndoToEpi")

# Creamos array para guardar la información Apex(valor 0) - Base(valor 2) - Resto ventriculo(valor 1) 
tagApexBase = vtk.vtkFloatArray()
tagApexBase.SetNumberOfComponents(1)
tagApexBase.SetName("tagApexBase")

# Creamos array para etiquetar en el modelo el tipo de celda sana, core,BZ (sana 0, core 2, BZ 1)
type_cell = vtk.vtkFloatArray()
type_cell.SetNumberOfComponents(1)
type_cell.SetName("Cell_type")

# Creamos array para guardar la información la orientación de fibras
fibers_OR = vtk.vtkFloatArray()
fibers_OR.SetNumberOfComponents(3)
fibers_OR.SetName("fibers_OR")

# Creamos array para guardar la información de las normales a endo
endo_Norm = vtk.vtkFloatArray()
endo_Norm.SetNumberOfComponents(3)
endo_Norm.SetName("endo_Norm")

# Creamos array para guardar la información de las normales a epi
epi_Norm = vtk.vtkFloatArray()
epi_Norm.SetNumberOfComponents(3)
epi_Norm.SetName("epi_Norm")

# Calculamos centro de masas de Ventriculo voxelizado para obtener Apex como celda de max distancia de centro a Epi 
centerFilter = vtk.vtkCenterOfMass()
centerFilter.SetInputData(dataVent)
centerFilter.SetUseScalarsAsWeights(False)
centerFilter.Update()
centerMass = centerFilter.GetCenter()

# Creamos array para guardar la información de las distancias Apex to Base de 0 a 1
distancesApex2Base = vtk.vtkFloatArray()
distancesApex2Base.SetNumberOfComponents(1)
distancesApex2Base.SetName("DistApexToBase")

# Evaluamos el signo de las distancias de cada coordenada de la celda del ventrículo
# respecto a la mesh del core y de la BZ (negativo-celda está dentro del core, 
# positivo-celda está fuera del core, 0-celda está en la superficie del core)
# y guardamos valores en array cellType (scar 2, BZ 1, sana 0)
# Inicializamos max distancia de celdas a centro
maxDistCenterCell = 0
#for pointId in range(dataVent.GetNumberOfPoints()):
for i in range(dataVent.GetNumberOfCells()):
    p = (x[i],y[i],z[i])
    # Evaluamos signo de la distancia al core(scar)
    signedDistanceCore = implicitPolyDataDistanceCore.EvaluateFunction(p)  
    # Evaluamos signo de la distancia al BZ
    signedDistanceBZ = implicitPolyDataDistanceBZ.EvaluateFunction(p) 
    # Evaluamos distancia en valor absoluto de cada centroide al punto más cercano a Epi 
    distancesEpi1 = implicitPolyDataDistanceEpi.EvaluateFunction(p)
    signedDistanceEpi = abs(distancesEpi1)
    distancesEpi.InsertNextValue(distancesEpi1)
    # Evaluamos distancia en valor absoluto de cada centroide al punto más cercano a Endo
    distancesEndo1 = implicitPolyDataDistanceEndo.EvaluateFunction(p)
    signedDistanceEndo = abs(distancesEndo1)
    distancesEndo.InsertNextValue(distancesEndo1)
    # Calculamos la distancia de Endo a Epi de 0 a 1 respectivamente, respecto al grosor del miocardio
    distanceEndo2Epi = (signedDistanceEndo / (signedDistanceEndo+signedDistanceEpi)) 

    # Calculamos la distancia de Endo a Epi de 1 a 0 respectivamente, respecto al grosor del miocardio
    #distanceEndo2Epi = (signedDistanceEpi / (signedDistanceEndo+signedDistanceEpi)) 

    # Guardamos el valor para guardar etiqueta y visualizar en modelo
    distancesEndo2Epi.InsertNextValue(distanceEndo2Epi)
    # Dependiendo de la distancia de Endo2Epi etiquetamos la celda como Endo, Mid, Epi (17%, 41% y 42% del grosor del ventrículo, respectivamente)
    # Etiquetado: endo 0, Mid 1, epi 2 
    # Y guardamos el valor para guardar etiqueta y visualizar en modelo
    # Endo
    if distanceEndo2Epi <= 0.17:
        Endo2Epi.InsertNextValue(0)
    # Mid
    elif distanceEndo2Epi <= 0.58:
        Endo2Epi.InsertNextValue(1)
    # Epi 
    else:
        Endo2Epi.InsertNextValue(2)
    # Si distancia negativa o 0 pertenece al core y la guardamos como scar 2
    if (signedDistanceCore <= 0):
        cellType[i] = 2
        # Lo guardamos en el modelo vtk para etiquetar el tipo de celda
        type_cell.InsertNextValue(2)
    # Si distancia negativa o 0 pertenece al BZ y la guardamos como BZ 1
    elif (signedDistanceBZ <= 0):
        cellType[i] = 1
        # Lo guardamos en el modelo vtk para etiquetar el tipo de celda 
        type_cell.InsertNextValue(1)
    # Si no, es tejido sano 0
    else:
        cellType[i] = 0
        # Lo guardamos en el modelo vtk para etiquetar el tipo de celda
        type_cell.InsertNextValue(0)

    # Calculamos la celda a distancia máxima desde centro de masas para obtener coordenada Apex,
    # con coordenada y menor que el centro para asegurar que está en la mitad inferior
    distSquared = abs(vtk.vtkMath.Distance2BetweenPoints(p,centerMass))
    if distSquared > maxDistCenterCell and p[2] < centerMass[2]:
        maxDistCenterCell = distSquared
        coordApex = p


# Creamos Polydata del Apex y de la Base para calcular distancias desde cada celda
## Apex
points = vtk.vtkPoints()
points.InsertNextPoint(coordApex[0]-0.5,coordApex[1]-0.5,coordApex[2])
points.InsertNextPoint(coordApex[0], coordApex[1], coordApex[2])
points.InsertNextPoint(coordApex[0]+0.5,coordApex[1]-0.5,coordApex[2])
polygon = vtk.vtkPolygon()
polygon.GetPointIds().SetNumberOfIds(3) 
polygon.GetPointIds().SetId(0,0)
polygon.GetPointIds().SetId(1,1)
polygon.GetPointIds().SetId(2,2)
polygons = vtk.vtkCellArray()
polygons.InsertNextCell(polygon)
polyApex = vtk.vtkPolyData()
polyApex.SetPoints(points)
polyApex.SetPolys(polygons) 
## Base
featureEdges = vtk.vtkFeatureEdges()
if vtk.vtkVersion().GetVTKMajorVersion() >5:
    featureEdges.SetInputData(mesh_readerEndo.GetOutput())
else:
    featureEdges.SetInput(mesh_readerEndo.GetOutput())
featureEdges.FeatureEdgesOff()
featureEdges.BoundaryEdgesOn()
featureEdges.NonManifoldEdgesOff()
featureEdges.ManifoldEdgesOff()
featureEdges.Update()

# Localizamos las celdas del ventrículo cercanas a la Base para etiquetarlas
cell_locator = vtk.vtkCellLocator()
cell_locator.SetDataSet(dataVent)  
cell_locator.BuildLocator()

points = featureEdges.GetOutput().GetPoints()
cellId_Base = []
for i in range(points.GetNumberOfPoints()):
    p = points.GetPoint(i)
    cellId = vtk.mutable(0)
    c = [0.0, 0.0, 0.0]
    subId = vtk.mutable(0)
    d = vtk.mutable(0.0)
    cell_locator.FindClosestPoint(p, c, cellId, subId, d)
    cellId_Base.append(cellId)
    
# Construímos Polydata de la base para usar en función implícita
boundaryStrips = vtk.vtkStripper()
boundaryStrips.SetInputConnection(featureEdges.GetOutputPort())
boundaryStrips.Update()

polyBase = vtk.vtkTubeFilter()
polyBase.SetInputData(boundaryStrips.GetOutput())
polyBase.SetRadius(1.5)
polyBase.Update()

# Función implícita para evaluar la distancia de cada celda a Apex
implicitPolyDataDistanceApex = vtk.vtkImplicitPolyDataDistance()
implicitPolyDataDistanceApex.SetInput(polyApex)

# Función implícita para evaluar la distancia de cada celda a Base
implicitPolyDataDistanceBase = vtk.vtkImplicitPolyDataDistance()
implicitPolyDataDistanceBase.SetInput(polyBase.GetOutput())

# Etiquetamos el ventrículo indicando la celda Apex con valor 0, las celdas de la Base con valor 2 y el resto 1
# Obtenemos la distancia de 0 a 1 de Apex to Base para cada celda 
# Y obtenemos el vector normal a la superficie Endo y el vector normal a la superficie Epi
for i in range(dataVent.GetNumberOfCells()):
    p = (x[i],y[i],z[i])
    #idCell = dataVent.GetCell(i)
    if p == coordApex:
        tagApexBase.InsertNextValue(0)
    elif i in cellId_Base:
        tagApexBase.InsertNextValue(2)
    else:
        tagApexBase.InsertNextValue(1)
    
    signedDistanceApex = abs(implicitPolyDataDistanceApex.EvaluateFunction(p))
    signedDistanceBase = abs(implicitPolyDataDistanceBase.EvaluateFunction(p))
    # Calculamos la distancia de Apex a Base de 0 a 1 respecto al grosor del miocardio
    # (DistApex / (DistApex+DistBase))
    distanceApex2Base = (signedDistanceApex / (signedDistanceApex+signedDistanceBase)) 

    # Calculamos la distancia de Apex a Base de 1 a 0 respecto al grosor del miocardio
    # (DistApex / (DistApex+DistBase))
    #distanceApex2Base = (signedDistanceBase / (signedDistanceApex+signedDistanceBase)) 

    # Guardamos el valor para guardar etiqueta y visualizar en modelo
    distancesApex2Base.InsertNextValue(distanceApex2Base)


                ####                                            ####
                ##         CÁLCULO ORIENTACIÓN DE FIBRAS          ##
                ####                                            ####

#### Calculamos el vector del eje longitudinal del ventriculo, entre el punto apex y el centroide de la base

# Calculamos el centroide de la base (mitral valve)
centerBase = vtk.vtkCenterOfMass()
centerBase.SetInputData(polyBase.GetOutput())
centerBase.SetUseScalarsAsWeights(False)
centerBase.Update()
centerMassBase = centerBase.GetCenter()

# Elevamos la coordenada z al valor máximo del ventrículo para que cubra todo el ventrículo
zmax = np.amax(z)
centerMassBaseList = list(centerMassBase)
centerMassBaseList[2] = zmax
centerMassBase = tuple(centerMassBaseList)
# Calculamos el vector del eje longitudinal del ventriculo de apex a base
vectorLongAxes = np.subtract(coordApex,centerMassBase)
#vectorLongAxes = np.subtract(centerMassBase,coordApex)
# Normalizamos 
#vectorLongAxesNorm = np.linalg.norm(vectorLongAxes)
#vectorLongAxes = vectorLongAxes/vectorLongAxesNorm

# Calculamos para cada celda el vector normal a Endo y a Epi y 
# vector de orientación de fibras
for i in range(dataVent.GetNumberOfCells()):
    # Coordenada de celda ventrículo
    p = (x[i],y[i],z[i])
    # Punto más cercano a Endo
    # Calculamos distancia para saber si está dentro o fuera de polydata para definir la resta al calcular el vector y obtenemos la coord del punto más ceracno
    endo_N_point = np.zeros(3)
    distanceEndo = implicitPolyDataDistanceEndoNorm.EvaluateFunctionAndGetClosestPoint(p, endo_N_point)  

    # Calculamos el vector de la celda del ventrículo al punto más cercano a Endo
    endo_N = np.zeros(3)
    # Si celda en zona epi y la distancia positiva o 0, el punto está fuera de la malla endo por los vóxeles que salen, 
    # en ese caso calculamos el vector restando al revés para que la dirección sea la correcta hacia endo o epi  
    if Endo2Epi.GetValue(i) == 0 and distanceEndo >= 0:
        endo_N = np.subtract(p, endo_N_point)
    else:
        endo_N = np.subtract(endo_N_point, p)
    # Normalizamos vector
    endoNorm = np.linalg.norm(endo_N)
    endo_N = endo_N/endoNorm

    # Punto más cercano a Epi
    # Calculamos distancia para saber si está dentro o fuera de polydata para definir la resta al calcular el vector y obtenemos la coord del punto más ceracno
    epi_N_point = np.zeros(3)
    distanceEpi = implicitPolyDataDistanceEpiNorm.EvaluateFunctionAndGetClosestPoint(p, epi_N_point)  

    # Calculamos el vector de la celda del ventrículo al punto más cercano a Epi
    epi_N = np.zeros(3)
    # Si la distancia positiva o 0, el punto está fuera de la malla endo por los vóxeles que salen, 
    # en ese caso calculamos el vector restando al revés para que la dirección sea la correcta hacia endo o epi  
    if Endo2Epi.GetValue(i) == 2 and distanceEpi >= 0:
        epi_N = np.subtract(p, epi_N_point)
    else:
        epi_N = np.subtract(epi_N_point, p)
    
    # Normalizamos vector
    epiNorm = np.linalg.norm(epi_N)
    epi_N = epi_N/epiNorm



    # Valor de 0 a 1 endo 2 epi
    w = distancesEndo2Epi.GetValue(i)
    apex2Base = distancesApex2Base.GetValue(i)
    phi = abs(np.arccos(apex2Base)) - (np.pi/2)

    alpha_h = -1.9 * w + 0.862 # OK Mid direction

    # Transmural angle STREETER
    alpha_t = -0.2149 * phi**2 + 0.0089 * phi - 0.0093 # OK Mid direction
    #alpha_t = 0.2149 * phi**2 + 0.0089 * phi - 0.0093

    # Fibre orientation vector
    FO = np.zeros(3)
    FO = [np.tan(alpha_t), 1, np.tan(alpha_h)]
    # Normalizamos vector
    FO_Norm = np.linalg.norm(FO)
    FO = FO/FO_Norm
    # Convertimos el vector de orientación de fibras de coordenadas globales a locales
    FO_local = global2localCoord(FO, epi_N, endo_N, w, vectorLongAxes,i)
    # Direccion de la orientacion de las fibras cardiacas en coordenadas por separado para guardar en txt's 
    fibreOrientationX[i]= FO_local[0,0]
    fibreOrientationY[i]= FO_local[0,1]
    fibreOrientationZ[i]= FO_local[0,2]
    
    # Lo guardamos también en Array para etiquetarlo en el modelo vtk
    epi_Norm.InsertNextTuple3(epi_N[0], epi_N[1], epi_N[2])
    endo_Norm.InsertNextTuple3(endo_N[0], endo_N[1], endo_N[2])
    fibers_OR.InsertNextTuple3(FO_local[0,0], FO_local[0,1], FO_local[0,2])

# Añadimos scalar para visualizar datos en modelo
dataVent.GetCellData().AddArray(distancesEndo2Epi)
dataVent.GetCellData().AddArray(Endo2Epi)
dataVent.GetCellData().AddArray(type_cell)
dataVent.GetCellData().AddArray(fibers_OR)
dataVent.GetCellData().AddArray(endo_Norm)
dataVent.GetCellData().AddArray(epi_Norm)
dataVent.GetCellData().AddArray(tagApexBase)
dataVent.GetCellData().AddArray(distancesApex2Base)
dataVent.Modified()

# Guardamos modelo etiquetado y voxelizado
writer = vtk.vtkUnstructuredGridWriter()
oname = f'{source}/ventricle_Tagged.vtk'
writer.SetFileName(oname)
writer.SetInputData(dataVent)
writer.Modified()
writer.Write()
print("  Created ventricle tagged.........................", oname)

# Guardamos malla stl para visualización base en Processing
writer2 = vtk.vtkSTLWriter()
writer2.SetFileName(f'{source}/Endo.stl')
writer2.SetFileTypeToBinary()
writer2.SetInputConnection(mesh_readerEndo.GetOutputPort())
writer2.Write()
print("  Created ventricle endo stl.........................", f'{source}/Endo.stl')

# Obtenemos todas las propiedades que contiene el archivo vtk a nivel elemento y modo
print('\nPROPIEDADES2 ASIGNADAS AL ELEMENTO\n')
for i in range(dataVent.GetCellData().GetNumberOfArrays()):
    print (dataVent.GetCellData().GetArrayName(i)+'\n')

print('\nPROPIEDADES2 ASIGNADAS AL MODO\n')
for i in range(dataVent.GetPointData().GetNumberOfArrays()):
    print (dataVent.GetPointData().GetArrayName(i)+'\n')



# Convertimos vtk Array to numpy para guardar en txt
endo2epi_np = vtk_to_numpy(Endo2Epi)
centerMass_np = centerMass[0], centerMass[1], centerMass[2]

path_ACData = f'{source}/AC_Data'
if not os.path.exists(path_ACData):
    os.mkdir(path_ACData)

#Guardamos en ficheros de texto por separado
np.savetxt(f'{path_ACData}/centroidCellX.txt',x, delimiter =', ',fmt='%f')
np.savetxt(f'{path_ACData}/centroidCellY.txt',y, delimiter =', ',fmt='%f')
np.savetxt(f'{path_ACData}/centroidCellZ.txt',z, delimiter =', ',fmt='%f')
np.savetxt(f'{path_ACData}/scalarValues.txt',scalar, delimiter =', ',fmt='%f')
np.savetxt(f'{path_ACData}/EndoToEpi.txt',endo2epi_np, delimiter =', ',fmt='%f')
np.savetxt(f'{path_ACData}/scarTissue.txt',cellType, delimiter =', ',fmt='%f')
np.savetxt(f'{path_ACData}/fibreOrientationX.txt',fibreOrientationX, delimiter =', ',fmt='%f')
np.savetxt(f'{path_ACData}/fibreOrientationY.txt',fibreOrientationY, delimiter =', ',fmt='%f')
np.savetxt(f'{path_ACData}/fibreOrientationZ.txt',fibreOrientationZ, delimiter =', ',fmt='%f')
np.savetxt(f'{path_ACData}/centerMass.txt',centerMass_np, delimiter =', ',fmt='%f')


#Guarda vecinos de cada celda 
fout = open(f'{path_ACData}/vecinos.txt','w')
for cellId in range(dataVent.GetNumberOfCells()):
    vs = vecinos(cellId)
    for v in vs:
        fout.write(str(v)+' ')
    fout.write('\n')

fout.close()


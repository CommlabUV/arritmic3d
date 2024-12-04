import vtk
import numpy as np
import os
from shutil import copy
from vtk.util.numpy_support import vtk_to_numpy


########   Requerimientos  ########
### Script preparado para modelos orientados en el eje z (0,0,1)
### Revisar que protocolo de nombres de archivos de segmentación y resonancia definidos en el código sea el correcto:
#       Carpeta caso p* debe contener:
#               - Study***.vtk (Archivo de resonancia, debe empezar por Study)
#               - Left Ventricle.vtk (polydata ventrículo entero cerrado)
#               - Core Surface.vtk
#               - Border Zone Surface.vtk
#               - Epi Layer.vtk
#               - Endo Layer.vtk
### Se necesita un archivo params.dat en la raíz de la carpeta del script para que se copie dentro del caso


######################################################
############### PARÁMETROS A DEFINIR #################
######################################################

# Definir rango de número de pacientes a generar (p*)
# Ej: para generar desde el caso p10 al p12 asignar minRange=10 maxRange=13
# IMP!! el número de caso definido en maxRange no se generará, se generará hasta el número anterior al mismo
minRange = 1
maxRange = 42

# Path a la carpeta que contiene las subcarpetas con los casos segmentados como p0, p1, p2...
sourceCase = f'VTK_Clara_Footprint_OK'

############# FIN PARÁMETROS A DEFINIR ###############



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

# Lista de casos con resolución no válida
resFail = []

# Definir rango de número de pacientes a generar (p*)
for case in range(minRange, maxRange):
    # Nombre archivos vtk
    source = f'{sourceCase}/p{case}'
    if os.path.isdir(source):
        print(f'CASO p{case}')
        filenameCore = f'{source}/Core Surface.vtk'
        filenameBZ = f'{source}/Border Zone Surface.vtk'
        filenameEpi = f'{source}/Epi Layer.vtk'
        filenameEndo = f'{source}/Endo Layer.vtk'

        #### Voxelizamos archivo de resonancia #####
        # Archivo de resonancia
        # Búscamos nombre de archivo resonancia porque cada caso añade un número aleatorio
        # detrás de Study
        for f_name in os.listdir(source):
            if f_name.startswith('Study'):
                print('Study File: ' , f_name)
                file_nameRes = f'{source}/{f_name}'
        # Malla ventrículo Izq
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
        # Comprobamos que la resolución esté dentro de un margen correcto, sino
        # añadimos el numero de caso en la lista para imprimirlo al final
        if xsizeCell > 1.5 or ysizeCell > 1.5 or zsizeCell > 1.5:
            resFail.append(f'p{case}')
            print(f'RESOLUCION NO VÁLIDA EN p{case}')
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

        # Copiamos datos de valor scalar a puntos
        for i in range(ugrid.GetOutput().GetNumberOfPoints()):
            scalarsVent.InsertNextValue(ugrid.GetOutput().GetPointData().GetArray('scalars').GetValue(i))

        ugrid.GetOutput().GetPointData().AddArray(scalarsVent)
        ugrid.Update()
        print("  Fin scalar PointData.........................")


        #Cargamos los datos del ventriculo voxelizado
        dataVent = ugrid.GetOutput()

        # Obtenemos todas las propiedades que contiene el archivo vtk a nivel elemento y nodo
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

        #Creamos 3 arrays distintos para coord nodos celda x, y, z.
        x = np.zeros(dataVent.GetNumberOfPoints())
        y = np.zeros(dataVent.GetNumberOfPoints())
        z = np.zeros(dataVent.GetNumberOfPoints())

        # Creamos 3 arrays distintos para orientación de fibras
        fibreOrientationX = np.zeros(dataVent.GetNumberOfPoints())
        fibreOrientationY = np.zeros(dataVent.GetNumberOfPoints())
        fibreOrientationZ = np.zeros(dataVent.GetNumberOfPoints())

        #Guardamos las coordenadas por separado
        for i in range(dataVent.GetNumberOfPoints()):
            x[i],y[i],z[i] = dataVent.GetPoint(i)

        #Creamos un array de los valores de intensidad de cada celda en el mismo orden que las coordenadas guardadas
        scalar = np.zeros(dataVent.GetNumberOfPoints())
        for i in range(dataVent.GetNumberOfPoints()):
            scalar[i]=dataVent.GetPointData().GetArray('scalars').GetValue(i)


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
        cellType = np.zeros(dataVent.GetNumberOfPoints())
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

        # Creamos array para guardar la información del vector Long Axes
        vector_LAxis = vtk.vtkFloatArray()
        vector_LAxis.SetNumberOfComponents(3)
        vector_LAxis.SetName("vector_LAxis")

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

        # Creamos array para etiquetar en el modelo los 17 segmentos AHA
        aha_17 = vtk.vtkFloatArray()
        aha_17.SetNumberOfComponents(1)
        aha_17.SetName("17_AHA")

        # Creamos array para etiquetar en el modelo los 17 nodos de pacing
        pacing_34 = vtk.vtkFloatArray()
        pacing_34.SetNumberOfComponents(1)
        pacing_34.SetName("34_pacing")

        # Creamos array para guardar uno de cada nodo de segmento AHA para obtener los 17 nodos de pacing
        pacing_34Nodes = np.ones(34)*(-1)

        # Evaluamos el signo de las distancias de cada coordenada de la celda del ventrículo
        # respecto a la mesh del core y de la BZ (negativo está dentro del core,
        # positivo está fuera del core, 0 está en la superficie del core)
        # y guardamos valores en array cellType (scar 2, BZ 1, sana 0)
        # Inicializamos max distancia de celdas a centro
        maxDistCenterCell = 0
        for i in range(dataVent.GetNumberOfPoints()):
            p = (x[i],y[i],z[i])
            # Evaluamos signo de la distancia al core(scar)
            signedDistanceCore = implicitPolyDataDistanceCore.EvaluateFunction(p)
            # Evaluamos signo de la distancia al BZ
            signedDistanceBZ = implicitPolyDataDistanceBZ.EvaluateFunction(p)
            # Evaluamos distancia en valor absoluto de cada coordenda al punto más cercano a Epi
            distancesEpi1 = implicitPolyDataDistanceEpi.EvaluateFunction(p)
            signedDistanceEpi = abs(distancesEpi1)
            distancesEpi.InsertNextValue(distancesEpi1)
            # Evaluamos distancia en valor absoluto de cada coordenda al punto más cercano a Endo
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
                # Lo guardamos en el modelo vtk para etiquetar el tipo de nodo
                type_cell.InsertNextValue(2)
            # Si distancia negativa o 0 pertenece al BZ y la guardamos como BZ 1
            elif (signedDistanceBZ <= 0):
                cellType[i] = 1
                # Lo guardamos en el modelo vtk para etiquetar el tipo de nodo
                type_cell.InsertNextValue(1)
            # Si no, es tejido sano 0
            else:
                cellType[i] = 0
                # Lo guardamos en el modelo vtk para etiquetar el tipo de nodo
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

        # Localizamos los nodos del ventrículo cercanos a la Base para etiquetarlos
        cell_locator = vtk.vtkCellLocator()
        cell_locator.SetDataSet(dataVent)
        cell_locator.BuildLocator()

        points = featureEdges.GetOutput().GetPoints()
        pointId_Base = []
        for i in range(points.GetNumberOfPoints()):
            p = points.GetPoint(i)
            cellId = vtk.mutable(0)
            c = [0.0, 0.0, 0.0]
            subId = vtk.mutable(0)
            d = vtk.mutable(0.0)
            cell_locator.FindClosestPoint(p, c, cellId, subId, d)

            cellPointIds = vtk.vtkIdList()
            dataVent.GetCellPoints(cellId, cellPointIds)
            for l in range (cellPointIds.GetNumberOfIds()):
                pointId_Base.append(cellPointIds.GetId(l))

        # Construímos Polydata de la base para usar en función implícita
        boundaryStrips = vtk.vtkStripper()
        boundaryStrips.SetInputConnection(featureEdges.GetOutputPort())
        boundaryStrips.Update()

        polyBase = vtk.vtkTubeFilter()
        polyBase.SetInputData(boundaryStrips.GetOutput())
        polyBase.SetRadius(1.5)
        polyBase.Update()

        # Función implícita para evaluar la distancia de cada nodo a Apex
        implicitPolyDataDistanceApex = vtk.vtkImplicitPolyDataDistance()
        implicitPolyDataDistanceApex.SetInput(polyApex)

        # Función implícita para evaluar la distancia de cada nodo a Base
        implicitPolyDataDistanceBase = vtk.vtkImplicitPolyDataDistance()
        implicitPolyDataDistanceBase.SetInput(polyBase.GetOutput())

        ###################################################################################
        ############################# 17 segmentos AHA ####################################
        ###################################################################################

        # Calculamos los datos necesarios para dividir el ventrículo en los 17 segmentos AHA
        #  Datos para obtener después el Vector x,y desde el centro de masas a la mitad de la pared septal (Los ventrículos
        #  están siempre orientados igual (el nodo con mínima coordenada x define la pared de unión del VI con el VD)
        indexMinX = np.argmin(x)
        print('Index min X:', indexMinX)
        coordSept = x[indexMinX], y[indexMinX]
        print('Coord min X:', coordSept)
        centerMass_XY = centerMass[0],centerMass[1]
        #  Calculamos las 3 alturas que dividen el ventrículo en las 4 partes Apex, Apical, Mid-Cavity, Basal
        #   Altura ventrículo
        altVent = np.amax(z) - np.amin(z)
        print('Alt Vent:', altVent)
        #   Altura de cada Apex+Apical, Mid-Cavity, Basal
        altH = altVent / 3
        print('Alt H:', altH)
        #   Buscamos el mínimo z de los nodos Endo para definir la altura máxima Apex
        hApex = np.amax(z)
        for i in range(dataVent.GetNumberOfPoints()):
            if Endo2Epi.GetValue(i) == 0 and z[i] < hApex and distancesEndo2Epi.GetValue(i) <= 0.01:
                hApex = z[i]
        print('hApex:', hApex)
        #   Altura max Apical
        hApical = np.amin(z) + altH
        #   Altura max Mid-Cavity
        hMidCavity = hApical + altH
        #   Los nodos definidos en Basal serán todos los que estén por encima de la altura max Mid-Cavity

        #   Contadores para localizar nodos pacing lo m´s centrados posible en cada segmento
        c=c1=c2=c3=c4=c5=c6=c7=c8=c9=c10=c11=c12=c13=c14=c15=c16 = 0

        # Etiquetamos el ventrículo indicando la celda Apex con valor 0, las celdas de la Base con valor 2 y el resto 1
        # Obtenemos la distancia de 0 a 1 de Apex to Base para cada celda
        # Obtenemos el vector normal a la superficie Endo y el vector normal a la superficie Epi
        # Calculamos el segmento 17 AHA al que pertenece cada punto
        for i in range(dataVent.GetNumberOfPoints()):
            p = (x[i],y[i],z[i])
            if p == coordApex:
                tagApexBase.InsertNextValue(0)
            elif i in pointId_Base:
                tagApexBase.InsertNextValue(2)
            else:
                tagApexBase.InsertNextValue(1)

            # Calculamos el segmento 17 AHA al que pertenece el punto
            #  Altura Apex
            if z[i] <= hApex:
                seg = 17
                c = 5001
            #  Altura Apical
            elif z[i] <= hApical:
                #  Calculamos el vector XY del centro de masas a pared septal
                vecIni = np.subtract(coordSept,centerMass_XY)
                # Rotamos (sentido horario) el vector center-septal al inicio del segmento 13
                ang13 = 5*np.pi/4
                vecIni = vecIni[0]*np.cos(ang13)-vecIni[1]*np.sin(ang13),vecIni[0]*np.sin(ang13)+vecIni[1]*np.cos(ang13)
                # Obtenemos elvector XY del centro de masas al punto
                coordP = x[i],y[i]
                vecP = np.subtract(coordP,centerMass_XY)
                # Calculamos el ángulo entre los dos vectores para definir el segmento al que pertenece
                angle = np.arctan2(vecIni[0] * vecP[1] - vecIni[1] * vecP[0], vecIni[0] * vecP[0] + vecIni[1] * vecP[1])
                # Corregimos los ángulos negativos para obtener valores de 0 a 2PI
                if (angle < 0):
                    angle += np.pi*2
                # Definimos cada segmento
                if (0 <= angle <= 1.57):
                    seg = 13
                    c13 += 1
                    c = c13
                elif (angle <= 3.14):
                    seg = 14
                    c14 += 1
                    c = c14
                elif (angle <= 4.71):
                    seg = 15
                    c15 += 1
                    c = c15
                else:
                    seg = 16
                    c16 += 1
                    c = c16
            # Si estamos en los dos primeros niveles Mid-Cavity, Basal
            else:
                #  Calculamos el vector XY del centro de masas a pared septal
                vecIni = np.subtract(coordSept,centerMass_XY)
                # Rotamos (sentido horario) el vector center-septal al inicio del segmento 1 y 7
                angsup = 4*np.pi/3
                vecIni = vecIni[0]*np.cos(angsup)-vecIni[1]*np.sin(angsup),vecIni[0]*np.sin(angsup)+vecIni[1]*np.cos(angsup)
                # Obtenemos elvector XY del centro de masas al punto
                coordP = x[i],y[i]
                vecP = np.subtract(coordP,centerMass_XY)
                # Calculamos el ángulo entre los dos vectores para definir el segmento al que pertenece
                angle = np.arctan2(vecIni[0] * vecP[1] - vecIni[1] * vecP[0], vecIni[0] * vecP[0] + vecIni[1] * vecP[1])
                # Corregimos los ángulos negativos para obtener valores de 0 a 2PI
                if (angle < 0):
                    angle += np.pi*2
                # Definimos cada segmento
                if (0 <= angle <= 1.05):
                    # Altura MidCavity
                    if z[i] <= hMidCavity:
                        seg = 7
                        c7 += 1
                        c = c7
                    # Altura Basal
                    else:
                        seg = 1
                        c1 += 1
                        c = c1
                elif (angle <= 2.1):
                    # Altura MidCavity
                    if z[i] <= hMidCavity:
                        seg = 8
                        c8 += 1
                        c = c8
                    # Altura Basal
                    else:
                        seg = 2
                        c2 += 1
                        c = c2

                elif (angle <= 3.14):
                    # Altura MidCavity
                    if z[i] <= hMidCavity:
                        seg = 9
                        c9 += 1
                        c = c9
                    # Altura Basal
                    else:
                        seg = 3
                        c3 += 1
                        c = c3
                elif (angle <= 4.2):
                    # Altura MidCavity
                    if z[i] <= hMidCavity:
                        seg = 10
                        c10 += 1
                        c = c10
                    # Altura Basal
                    else:
                        seg = 4
                        c4 += 1
                        c = c4
                elif (angle <= 5.24):
                    # Altura MidCavity
                    if z[i] <= hMidCavity:
                        seg = 11
                        c11 += 1
                        c = c11
                    # Altura Basal
                    else:
                        seg = 5
                        c5 += 1
                        c = c5
                elif (angle <= 6.3):
                    # Altura MidCavity
                    if z[i] <= hMidCavity:
                        seg = 12
                        c12 += 1
                        c = c12
                    # Altura Basal
                    else:
                        seg = 6
                        c6 += 1
                        c = c6
            aha_17.InsertNextValue(seg)
            # Guardamos el primer nodo a partir de los 5000 (para que centre el nodo en el segmento) del segmento Endo y no Core como nodo de pacing en array y para etiquetar en modelo vtk
            if (pacing_34Nodes[seg-1] == -1 and c > 5000 and Endo2Epi.GetValue(i) == 0 and type_cell.GetValue(i) != 2):
                pacing_34Nodes[seg-1]=i
                pacing_34.InsertNextValue(1)
            # Guardamos el primer nodo a partir de los 5000 (para que centre el nodo en el segmento) del segmento Epi y no Core como nodo de pacing en array y para etiquetar en modelo vtk
            elif (pacing_34Nodes[16+seg] == -1 and c > 5000 and Endo2Epi.GetValue(i) == 2 and type_cell.GetValue(i) != 2):
                pacing_34Nodes[16+seg]=i
                pacing_34.InsertNextValue(1)
            else:
                # Etiquetamos el nodo como no pacing
                pacing_34.InsertNextValue(0)

            # FIN 17 segmentos AHA
            ######################################################


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

        # Calculamos para cada nodo el vector normal a Endo y a Epi y
        # vector de orientación de fibras
        for i in range(dataVent.GetNumberOfPoints()):
            # Coordenada de nodo ventrículo
            p = (x[i],y[i],z[i])
            # Punto más cercano a Endo
            # Calculamos distancia para saber si está dentro o fuera de polydata para definir la resta al calcular el vector y obtenemos la coord del punto más ceracno
            endo_N_point = np.zeros(3)
            distanceEndo = implicitPolyDataDistanceEndoNorm.EvaluateFunctionAndGetClosestPoint(p, endo_N_point)

            # Calculamos el vector del nodo del ventrículo al punto más cercano a Endo
            endo_N = np.zeros(3)
            # Si nodo en zona epi y la distancia positiva o 0, el punto está fuera de la malla endo por los vóxeles que salen,
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

            # Calculamos el vector del nodo del ventrículo al punto más cercano a Epi
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

            # Helix angle STREETER
            #alpha_h = 1.9 * w + 0.862
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
            vector_LAxis.InsertNextTuple3(vectorLongAxes[0], vectorLongAxes[1], vectorLongAxes[2])


        # Añadimos scalar para visualizar datos en modelo
        dataVent.GetPointData().AddArray(distancesEndo2Epi)
        dataVent.GetPointData().AddArray(Endo2Epi)
        dataVent.GetPointData().AddArray(type_cell)
        dataVent.GetPointData().AddArray(fibers_OR)
        dataVent.GetPointData().AddArray(endo_Norm)
        dataVent.GetPointData().AddArray(epi_Norm)
        dataVent.GetPointData().AddArray(vector_LAxis)
        dataVent.GetPointData().AddArray(tagApexBase)
        dataVent.GetPointData().AddArray(distancesApex2Base)
        dataVent.GetPointData().AddArray(aha_17)
        dataVent.GetPointData().AddArray(pacing_34)
        dataVent.Modified()

        # Guardamos modelo etiquetado y voxelizado en carpeta caso
        path_ACCase = f'{source}/Berruezo_p{case}'
        if not os.path.exists(path_ACCase):
            os.mkdir(path_ACCase)

        writer = vtk.vtkUnstructuredGridWriter()
        oname = f'{path_ACCase}/ventricle_Tagged.vtk'
        writer.SetFileName(oname)
        writer.SetInputData(dataVent)
        writer.Modified()
        writer.Write()
        print("  Created ventricle tagged.........................", oname)

        # Guardamos malla stl para visualización base en Processing
        writer2 = vtk.vtkSTLWriter()
        writer2.SetFileName(f'{path_ACCase}/Endo.stl')
        writer2.SetFileTypeToBinary()
        writer2.SetInputConnection(mesh_readerEndo.GetOutputPort())
        writer2.Write()
        print("  Created ventricle endo stl.........................", f'{path_ACCase}/Endo.stl')

        # Obtenemos todas las propiedades que contiene el archivo vtk a nivel elemento y modo
        print('\nPROPIEDADES2 ASIGNADAS AL ELEMENTO\n')
        for i in range(dataVent.GetCellData().GetNumberOfArrays()):
            print (dataVent.GetCellData().GetArrayName(i)+'\n')

        print('\nPROPIEDADES2 ASIGNADAS AL MODO\n')
        for i in range(dataVent.GetPointData().GetNumberOfArrays()):
            print (dataVent.GetPointData().GetArrayName(i)+'\n')

        # Copiamos archivo params.dat en carpeta caso
        copy('params.dat', f"{path_ACCase}/params.dat")

        # Convertimos vtk Array to numpy para guardar en txt
        endo2epi_np = vtk_to_numpy(Endo2Epi)
        centerMass_np = centerMass[0], centerMass[1], centerMass[2]

        path_ACData = f'{path_ACCase}/Reader_VTK'
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
        np.savetxt(f'{path_ACData}/centerMass-BPL.txt',centerMass_np, delimiter =', ',fmt='%f')
        np.savetxt(f'{path_ACData}/pacing_34.txt',pacing_34Nodes, delimiter =', ',fmt='%f')



        # Guardamos vecinos en archivo txt
        fout = open(f'{path_ACData}/vecinos.txt','w')
        # Creamos lista de puntos que contendrá lista de nodos vecinos para cada nodo
        nodos = [[] for _ in range(dataVent.GetNumberOfPoints())]
        for cellId in range(dataVent.GetNumberOfCells()):
            cellPointIds = vtk.vtkIdList()
            dataVent.GetCellPoints(cellId, cellPointIds)
            for l in range (cellPointIds.GetNumberOfIds()):
                nodoactual = cellPointIds.GetId(l)
                for i in range (cellPointIds.GetNumberOfIds()):
                    neigh = cellPointIds.GetId(i)
                    # Evitamos añadir el mismo nodo como vecino
                    if nodoactual != neigh:
                        # Comprobamos que el nodo vecino no se haya añadido desde otra celda y evitar que se duplique en la lista
                        if neigh not in nodos[nodoactual]:
                            nodos[nodoactual].append(neigh)


        # Copiamos datos de vecinos en txt
        for i in range(len(nodos)):
            for v in nodos[i]:
                fout.write(str(v)+' ')
            fout.write('\n')

        fout.close()

        # Guardamos los nodos conectados a cada celda para crear desde AC el archivo .geo para guardar la animación .case
        fout2 = open(f'{path_ACData}/cell_conex_nodos.txt','w')
        # Creamos lista de elemntos que contendrá lista de nodos conectados a cada elemento
        nodos = [[] for _ in range(dataVent.GetNumberOfCells())]
        for cellId in range(dataVent.GetNumberOfCells()):
            cellPointIds = vtk.vtkIdList()
            dataVent.GetCellPoints(cellId, cellPointIds)
            for l in range (cellPointIds.GetNumberOfIds()):
                nodoactual = cellPointIds.GetId(l)
                nodos[cellId].append(nodoactual)

        # Copiamos datos a txt
        for i in range(len(nodos)):
            for v in nodos[i]:
                fout2.write(str(v)+' ')
            fout2.write('\n')

        fout2.close()

print('CASOS CON RESOLUCION NO VÁLIDA:', resFail)



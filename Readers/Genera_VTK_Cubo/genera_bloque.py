#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import vtk
import os
import numpy as np
from shutil import copy
import random

######################################################
###### PARÁMETROS A DEFINIR PARA CADA CASO #######
######################################################

##### Requerimiento!!!! Se necesita un archivo params.dat en la raíz de la carpeta del script para que se copie dentro del caso

# Variables para etiquetar zonas Sanas, BZ y Core en número de celdas
cellsBySideX = 166 # Num celdas en X
cellsBySideY = 166 # Num celdas en Y
cellsBySideZ = 1 # Num celdas en Z (Si etiquetado canales, no usar valor 2 porque no generará la capa interior del canal)
cellSize = 0.3  # El AC funciona en mm

### ETIQUETADO CANALES genera geometría con canal de conducción lenta de BZ rodeado de tejido Core
etiquetar_canales = False # Etiquetado para generar geometría con canal de conducción lenta de BZ rodeado de tejido Core
## Variables a definir si etiquetado canales
altSano = 20    # Num celdas a lo alto de los dos bloques sanos
altBZ = 25      # Num celdas a lo alto de los dos bloques BZ
anchoCanal = 3  # Num celdas ancho canal en x. El ancho de canal en z depende del número de capas,
                # siempre deja dos capas exteriores y el resto pertenecn al canal... Ej: capas 5 deja 3 capas de canal, capas 4 deja 2 de canal


#### Sino etiquetamos canales, ETIQUETADO HOMOGENEO genera todo el bloque con tejido homogéneo. Mismo tipo de celda y conductividad.
# Opciones a definir:
    # endo2epi (tipo de celda) -> endo 0, mid 1, epi 2
    # tissue (conductividad)  -> sana 0, BZ 1, core 2
endo2epi = 0
tissue = 0

###### FIN PARÁMETROS A DEFINIR PARA CADA CASO #######



# Salida
str_canales = ""
if etiquetar_canales:
    str_canales = "_Canales"
caseName = f"./Bloque_vtk_{cellsBySideX:03}x{cellsBySideY:03}x{cellsBySideZ:03}{str_canales}"


capa1 = False
if cellsBySideZ == 1:
    capa1 = True

altCore = cellsBySideY - (2*altSano+2*altBZ)
altIniBZ1 = altSano -1
altIniCore = altSano + altBZ -1
altIniBZ2 = altCore + altSano + altBZ -1
altSano2 = altCore + altSano + 2*altBZ -1
anchoCanalIni1 = cellsBySideX/2-anchoCanal/2-1
anchoCanalIni2 = anchoCanalIni1+anchoCanal


max_x = cellsBySideX+1
max_y = cellsBySideY+1
max_z = cellsBySideZ+1
scale = cellSize
meshPoints = vtk.vtkPoints()
meshPoints.SetNumberOfPoints(max_x*max_y*max_z)
for k in range(max_z):
    for j in range(max_y):
        for i in range(max_x):
            # Celdas irregulares. Aleatoriamente movemos las coordenadas en X e Y de 0 a 0.01 random de más
            #meshPoints.InsertPoint(i*(max_y)*(max_z)+j*(max_z)+k,scale*i+(round(random.uniform(0.00, 0.01),4)),scale*j+(round(random.uniform(0.00, 0.01),4)),scale*k)
            # Celdas regulares
            meshPoints.InsertPoint(i*(max_y)*(max_z)+j*(max_z)+k,scale*i,scale*j,scale*k)



nelements = (max_x-1)*(max_y-1)*(max_z-1)
data = vtk.vtkUnstructuredGrid()
data.Allocate(nelements,nelements)
data.SetPoints(meshPoints)
for k in range(max_z-1):
    for j in range(max_y-1):
        for i in range(max_x-1):
            cell = vtk.vtkHexahedron()
            cell.GetPointIds().SetId(0, (i+1)*(max_y)*(max_z)+j*(max_z)+k)       # upper left front
            cell.GetPointIds().SetId(1, (i+1)*(max_y)*(max_z)+(j+1)*(max_z)+k)   # upper right front
            cell.GetPointIds().SetId(2, i*(max_y)*(max_z)+(j+1)*(max_z)+k)       # lower right front
            cell.GetPointIds().SetId(3, i*(max_y)*(max_z)+j*(max_z)+k)           # lower left front node index
            cell.GetPointIds().SetId(4, (i+1)*(max_y)*(max_z)+j*(max_z)+k+1)     # upper left back
            cell.GetPointIds().SetId(5, (i+1)*(max_y)*(max_z)+(j+1)*(max_z)+k+1) # upper right back
            cell.GetPointIds().SetId(6, i*(max_y)*(max_z)+(j+1)*(max_z)+k+1)     # lower right back
            cell.GetPointIds().SetId(7, i*(max_y)*(max_z)+j*(max_z)+k+1)         # lower left back

            data.InsertNextCell(cell.GetCellType(), cell.GetPointIds())


# Número de elementos del bloque
countCeldas = data.GetNumberOfCells()
# Número de nodos del bloque
countPuntos = data.GetNumberOfPoints()
print('COUNT CELLS - POINTS: ', countCeldas, countPuntos)




###################################################
### Obtenemos las coordenadas de todos los nodos  ###
###################################################

# Creamos 3 arrays distintos para coord de cada nodo x, y, z
x = np.zeros(countPuntos)
y = np.zeros(countPuntos)
z = np.zeros(countPuntos)

# Copiamos las coordenadas de cada nodo
for i in range(countPuntos):
        x[i],y[i],z[i] = data.GetPoint(i)




##############################################################################
### Creamos arrays para etiquetar el modelo vtk y generar txt's para el AC ###
##############################################################################

# Creamos array para guardar la conductividad de cada nodo para txt AC (sana 0, core 2, BZ 1)
scarTissue = np.zeros(countPuntos)

# Creamos array para guardar el tipo de nodo para txt AC (epi 2, Mid 1, endo 0  para AC)
EndoToEpi = np.zeros(countPuntos)
for i in range(countPuntos):
        scarTissue[i] = 3
        EndoToEpi[i] = 3

# Creamos array para etiquetar en el modelo vtk por elementos la conductividad de la celda (sana 0, core 2, BZ 1)
type_cell = vtk.vtkFloatArray()
type_cell.SetNumberOfComponents(1)
type_cell.SetName("Cell_type")

# Creamos array para etiquetar en el modelo vtk por nodos el tipo de célula (epi 2, Mid 1, endo 0)
EndoToEpi_cell = vtk.vtkFloatArray()
EndoToEpi_cell.SetNumberOfComponents(1)
EndoToEpi_cell.SetName("Endo2Epi")




############################################################
### Creación de los canales                              ###
############################################################

if etiquetar_canales:
    ## Esto solo es si tenemos distribución de canales con tejido no homogéneo para generar geometría con canal
    ## de conducción lenta de BZ rodeado de tejido Core
    # Tipo de celda (EndoToEpi,EndoToEpi_cell): endo 0, mid 1, epi 2
    # Conductividad: (scarTissue,type_cell): sana 0, BZ 1, core 2
    for k in range(cellsBySideZ):
        for j in range(cellsBySideY):
            for i in range(cellsBySideX):
                # Sanas
                if j <  altIniBZ1:
                    type_cell.InsertNextValue(0)
                    cellPointIds = vtk.vtkIdList()
                    data.GetCellPoints(j*cellsBySideX+i+(k*cellsBySideX*cellsBySideY), cellPointIds)
                    for l in range (cellPointIds.GetNumberOfIds()):
                        EndoToEpi_cell.InsertValue(cellPointIds.GetId(l),0)
                        scarTissue[cellPointIds.GetId(l)] = 0
                        EndoToEpi[cellPointIds.GetId(l)] = 0
                # BZ 1
                elif j <  altIniCore:
                    type_cell.InsertNextValue(1)
                    cellPointIds = vtk.vtkIdList()
                    data.GetCellPoints(j*cellsBySideX+i+(k*cellsBySideX*cellsBySideY), cellPointIds)
                    for l in range (cellPointIds.GetNumberOfIds()):
                        EndoToEpi_cell.InsertValue(cellPointIds.GetId(l),0)
                        scarTissue[cellPointIds.GetId(l)] = 1
                        EndoToEpi[cellPointIds.GetId(l)] = 0
                # Core en z=0 y z=2, en z=1 Core y canal BZ
                elif j <  altIniBZ2:
                    # Si capas interiores
                    if (k > 0 and k < (cellsBySideZ-1)) or capa1:
                        # Core
                        if i < anchoCanalIni1:
                            type_cell.InsertNextValue(2)
                            cellPointIds = vtk.vtkIdList()
                            data.GetCellPoints(j*cellsBySideX+i+(k*cellsBySideX*cellsBySideY), cellPointIds)
                            for l in range (cellPointIds.GetNumberOfIds()):
                                EndoToEpi_cell.InsertValue(cellPointIds.GetId(l),0)
                                scarTissue[cellPointIds.GetId(l)] = 2
                                EndoToEpi[cellPointIds.GetId(l)] = 0
                        # BZ
                        elif i < anchoCanalIni2:
                            type_cell.InsertNextValue(1)
                            cellPointIds = vtk.vtkIdList()
                            data.GetCellPoints(j*cellsBySideX+i+(k*cellsBySideX*cellsBySideY), cellPointIds)
                            for l in range (cellPointIds.GetNumberOfIds()):
                                EndoToEpi_cell.InsertValue(cellPointIds.GetId(l),0)
                                scarTissue[cellPointIds.GetId(l)] = 1
                                EndoToEpi[cellPointIds.GetId(l)] = 0
                        # Core
                        else:
                            type_cell.InsertNextValue(2)
                            cellPointIds = vtk.vtkIdList()
                            data.GetCellPoints(j*cellsBySideX+i+(k*cellsBySideX*cellsBySideY), cellPointIds)
                            for l in range (cellPointIds.GetNumberOfIds()):
                                EndoToEpi_cell.InsertValue(cellPointIds.GetId(l),0)
                                scarTissue[cellPointIds.GetId(l)] = 2
                                EndoToEpi[cellPointIds.GetId(l)] = 0
                    # Si capas exteriores todo Core
                    else:
                        type_cell.InsertNextValue(2)
                        cellPointIds = vtk.vtkIdList()
                        data.GetCellPoints(j*cellsBySideX+i+(k*cellsBySideX*cellsBySideY), cellPointIds)
                        for l in range (cellPointIds.GetNumberOfIds()):
                            EndoToEpi_cell.InsertValue(cellPointIds.GetId(l),0)
                            scarTissue[cellPointIds.GetId(l)] = 2
                            EndoToEpi[cellPointIds.GetId(l)] = 0

                # BZ 2
                elif j <  altSano2:
                    type_cell.InsertNextValue(1)
                    cellPointIds = vtk.vtkIdList()
                    data.GetCellPoints(j*cellsBySideX+i+(k*cellsBySideX*cellsBySideY), cellPointIds)
                    for l in range (cellPointIds.GetNumberOfIds()):
                        EndoToEpi_cell.InsertValue(cellPointIds.GetId(l),0)
                        scarTissue[cellPointIds.GetId(l)] = 1
                        EndoToEpi[cellPointIds.GetId(l)] = 0
                # Sanas
                else:
                    type_cell.InsertNextValue(0)
                    cellPointIds = vtk.vtkIdList()
                    data.GetCellPoints(j*cellsBySideX+i+(k*cellsBySideX*cellsBySideY), cellPointIds)
                    for l in range (cellPointIds.GetNumberOfIds()):
                        EndoToEpi_cell.InsertValue(cellPointIds.GetId(l),0)
                        scarTissue[cellPointIds.GetId(l)] = 0
                        EndoToEpi[cellPointIds.GetId(l)] = 0

# Si no etiquetamos canales, etiquetamos todo el bloque con tejido homogéneo en Endo2Epi y Tipo de celda
# Valores definidos en los parámetros de inicio
else:
    for i in range(countCeldas):
        type_cell.InsertNextValue(tissue)
        cellPointIds = vtk.vtkIdList()
        data.GetCellPoints(i,cellPointIds)
        for l in range (cellPointIds.GetNumberOfIds()):
            EndoToEpi_cell.InsertValue(cellPointIds.GetId(l),endo2epi)
            scarTissue[cellPointIds.GetId(l)] = tissue
            EndoToEpi[cellPointIds.GetId(l)] = endo2epi

# Calculamos las coordenadas del centro de masas para ajustar automáticamente la cámara en AC
centerFilter = vtk.vtkCenterOfMass()
centerFilter.SetInputData(data)
centerFilter.SetUseScalarsAsWeights(False)
centerFilter.Update()
centerMass = centerFilter.GetCenter()



#############################################################################
### Guardamos todos los datos en archivos de texto por separado par el AC ###
#############################################################################

# Guardamos en un mismo archivo: Cooordenadas centro de masas, num de celdas en X, Y, Z y tamaño de celda
centerMass_bpl_cellsize = centerMass[0], centerMass[1], centerMass[2], cellsBySideX, cellsBySideY, cellsBySideZ, cellSize

if not os.path.exists(caseName):
    os.mkdir(caseName)
if not os.path.exists(caseName+"/Reader_VTK"):
    os.mkdir(caseName+"/Reader_VTK")
print(" Saving tissue files...")
np.savetxt(f"{caseName}/Reader_VTK/scarTissue.txt", scarTissue, delimiter =', ', fmt='%f')
np.savetxt(f"{caseName}/Reader_VTK//EndoToEpi.txt", EndoToEpi, delimiter =', ', fmt='%f')
np.savetxt(f"{caseName}/Reader_VTK//centroidCellX.txt", x, delimiter =', ', fmt='%f')
np.savetxt(f"{caseName}/Reader_VTK//centroidCellY.txt", y, delimiter =', ', fmt='%f')
np.savetxt(f"{caseName}/Reader_VTK//centroidCellZ.txt", z, delimiter =', ', fmt='%f')
np.savetxt(f'{caseName}/Reader_VTK//centerMass-BPL.txt', centerMass_bpl_cellsize, delimiter =', ', fmt='%f')
print("            ....done.")


# Copiamos archivo params.dat en carpeta caso
copy('params.dat', f"{caseName}/params.dat")

# Guardamos vecinos en archivo txt
# Creamos lista de puntos que contendrá lista de nodos vecinos para cada nodo
nodos = [[] for _ in range(data.GetNumberOfPoints())]
fout = open(f"{caseName}/Reader_VTK/vecinos.txt",'w')
print('VECINOS')

for cellId in range(data.GetNumberOfCells()):
    cellPointIds = vtk.vtkIdList()
    data.GetCellPoints(cellId, cellPointIds)
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
count = 0
for i in range(len(nodos)):
    for v in nodos[i]:
        fout.write(str(v)+' ')
    fout.write('\n')
    count += 1

fout.close()

# Guardamos los nodos conectados a cada celda para crear desde AC el archivo .geo para guardar la animación .case
fout2 = open(f'{caseName}/Reader_VTK/cell_conex_nodos.txt','w')

# Creamos lista de elementos que contendrá lista de nodos conectados a cada elemento
nodos = [[] for _ in range(data.GetNumberOfCells())]
for cellId in range(data.GetNumberOfCells()):
    cellPointIds = vtk.vtkIdList()
    data.GetCellPoints(cellId, cellPointIds)
    for l in range (cellPointIds.GetNumberOfIds()):
        nodoactual = cellPointIds.GetId(l)
        nodos[cellId].append(nodoactual)

# Copiamos datos a txt
for i in range(len(nodos)):
    for v in nodos[i]:
        fout2.write(str(v)+' ')
    fout2.write('\n')

fout2.close()

# Añadimos arrays para visualizar datos en modelo en nodos
data.GetCellData().AddArray(type_cell)
data.GetPointData().AddArray(EndoToEpi_cell)
data.Modified()

# Obtenemos todas las propiedades que contiene el archivo vtk a nivel elemento y nodo
print('\nPROPIEDADES ASIGNADAS AL ELEMENTO\n')
for i in range(data.GetCellData().GetNumberOfArrays()):
    print (data.GetCellData().GetArrayName(i)+'\n')

print('\nPROPIEDADES ASIGNADAS AL NODO\n')
for i in range(data.GetPointData().GetNumberOfArrays()):
    print (data.GetPointData().GetArrayName(i)+'\n')

# Guardamos modelo vtk etiquetado y voxelizado
writer = vtk.vtkUnstructuredGridWriter()
oname = f"{caseName}/Bloque_Tagged.vtk"
writer.SetFileName(oname)
writer.SetInputData(data)
writer.Modified()
writer.Write()
print("  Created bloque tagged.........................", oname)

# Guardamos malla stl para visualización base en Processing
surface_filter = vtk.vtkDataSetSurfaceFilter()
surface_filter.SetInputData(data)

triangle_filter = vtk.vtkTriangleFilter()
triangle_filter.SetInputConnection(surface_filter.GetOutputPort())

##decimate triangle
deci = vtk.vtkDecimatePro()
deci.SetInputConnection(triangle_filter.GetOutputPort())
deci.SetTargetReduction(0.9)
deci.PreserveTopologyOn()

writer2 = vtk.vtkSTLWriter()
writer2.SetFileName(f"{caseName}/Bloque.stl")
writer2.SetFileTypeToBinary()
writer2.SetInputConnection(deci.GetOutputPort())
writer2.Write()
print("  Created bloque stl.........................", 'Bloque.stl')


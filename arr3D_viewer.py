import os
from natsort import natsorted
import pyvista as pv
from pyvistaqt import BackgroundPlotter
from qtpy.QtWidgets import QApplication
from qtpy.QtCore import QTimer
from qtpy.QtWidgets import QAction
from arr3D_config import check_directory, load_config_file
import numpy as np

class DiffusionVisualizer:
    def __init__(self, directory, config):
        self.path = directory
        self.config = load_config_file(config)
        try:
            self.pacing_site_id = int(directory.split('_')[-1].split('/')[0])
        except:
            self.pacing_site_id = ""  # Valor por defecto si no se puede extraer el ID
        
        self.thresholded_meshes = []
        self.current_frame = 0
        self.animation_running = False
        self.animation_speed = 0.1  # segundos entre frames
        self.mesh_name = "diffusion_mesh"
        self.current_mesh = None  # Para mantener referencia al mesh actual
        self.vis_mode = {0:'State', 1:'APD',2:'CV'}  # Modo de visualización (0: State, 1: APD, 2: DI,..)
        self.current_mode = 0  # Modo de visualización actual
        self.actor_vmode_txt = None  # Texto del modo de visualización
        self.opacity = 0  # Transparencia inicial
        self.clims = {'State': [0, 2], 'APD': [100, 400], 'CV': [0, 1]}  # Rango de colores para cada modo
        self.showing_initial_node = False  # Controla si se muestra el nodo inicial
        self.initial_node_actor = None
        self.inital_node_id = -1  # ID del nodo inicial, se puede cambiar según la configuración
        self.setup_data()
        self.setup_ui()
        
    def setup_data(self):
        """Carga y procesa todos los datos VTK"""
        if not os.path.isdir(self.path):
            raise ValueError(f"El directorio {self.path} no existe")

        vtk_files = [f for f in os.listdir(self.path) if f.endswith('.vtk')]
        if not vtk_files:
            raise ValueError(f"No se encontraron archivos .vtk en {self.path}")

        vtk_files = natsorted(vtk_files)
        file_paths = [os.path.join(self.path, f) for f in vtk_files]

        print("Cargando y procesando archivos...")
        for file_path in file_paths:
            print(f"Cargando {file_path}...")
            grid = pv.read(file_path)
            grid["original_id"] = np.arange(grid.n_points)
            if 'State' not in grid.point_data or 'Cell_type' not in grid.point_data:
                raise ValueError(f"Faltan campos en {file_path}")
            
            thresholded = grid.threshold([0,  2], scalars='Cell_type', all_scalars=True)
            # Obtener los IDs originales de los puntos seleccionados
            ids_originales = thresholded["original_id"]

            self.inital_node_id = np.where(ids_originales == self.config['INITIAL_NODE_ID'])[0]
        
            self.thresholded_meshes.append(thresholded)
    
        print(f"{len(self.thresholded_meshes)} archivos cargados correctamente")
    
    def setup_ui(self):
        """Configura la interfaz gráfica"""
        self.plotter = BackgroundPlotter(window_size=(1200, 900))
        self.plotter.set_background('white')
        self.plotter.enable_anti_aliasing()
        self.plotter.app_window.setWindowTitle("Arritmic 3D Viewer - ComMLab@uv.es")
        
        # Obtener la barra de menú
        menu_bar = self.plotter.app_window.menuBar()

        # Crear un nuevo menú "Simulations"
        simulation_menu = menu_bar.addMenu("Arritmic3D")

        # Acción: Lanzar Simulación
        launch_action = QAction("Run Simulation", self.plotter.app_window)
        launch_action.triggered.connect(self.run_simulation)
        simulation_menu.addAction(launch_action)

        # Añadir el primer mesh
        self.current_mesh = self.thresholded_meshes[0]
        self.actor = self.plotter.add_mesh(
            self.current_mesh,
            scalars=self.vis_mode[self.current_mode],
            name=self.mesh_name,
            show_scalar_bar=True,
            scalar_bar_args = {'vertical': True},
            opacity=self.opacity / 100,  # Convertir a rango [0, 1]
            cmap='coolwarm',
            clim=self.clims[self.vis_mode[self.current_mode]]  # Ajustar el rango de colores
        )
        
        self.plotter.add_text(
            "'a': Start/Stop Animation  \n"\
            "'m': Change vis-mode  \n"\
            "'i': Remove Frame slider  \n"\
            "'k': Show initial node  \n"\
            "'right/left': advance/go back 1 frame ",
            
            position='upper_left',
            font_size=10
        )
        self.actor_vmode_txt = self.plotter.add_text(
            f"Mode: {self.vis_mode[self.current_mode]}",
            position='upper_right',
            font_size=12,
            name='actor_vmode_txt'

        )

        # Configurar controles
        self.plotter.add_key_event('a', self.toggle_animation)
        self.plotter.add_key_event('q', self.plotter.close)
        self.plotter.add_key_event('Right', self.Right)
        self.plotter.add_key_event('Left', self.Left)
        self.plotter.add_key_event('m', self.Change_VisMode)
        self.plotter.add_key_event('k', self.inital_node)
        
        # Configurar slider con callback mejorado
        self.slider = self.plotter.add_slider_widget(
            self.slider_callback,
            [0, len(self.thresholded_meshes)-1],
            value=0,
            title='Frame',
            style='modern',
            pointa=(0.1, 0.1),
            pointb=(0.9, 0.1))
        
        self.opacity_slider = self.plotter.add_slider_widget(
            self.opacity_callback,
            [0, 100],
            value=40,
            title='Opacity',
            style='modern',
            pointa=(0.8, 0.9),
            pointb=(1.0, 0.9)
            )
        
        
    def run_simulation(self):
        
        # Elimina los ficheros actuales del directorio de salida
        print("Eliminando ficheros .vtk del directorio de salida...")
        vtk_files = [f for f in os.listdir(self.path) if f.endswith('.vtk')]
        for file in vtk_files:
            file_path = os.path.join(self.path, file)
            print(f"Eliminando {file_path}...")
            os.remove(file_path)

        print("Lanzando simulación...")
        command = f"python3.11 arritmic3D.py {self.path} "
        print(f"Executing command: {command}")
        # Execute the command to view the pacing site
        os.system(command)
        
        print("Re-loading simulation data.")
        self.thresholded_meshes.clear()
        self.current_frame = 0

        self.setup_data()
        self.plotter.remove_actor(self.mesh_name)
        self.current_mesh = self.thresholded_meshes[0]
        self.actor = self.plotter.add_mesh(
            self.current_mesh,
            scalars=self.vis_mode[self.current_mode],
            name=self.mesh_name,
            opacity=self.opacity / 100,  # Convertir a rango [0, 1]
            show_scalar_bar=True,
            scalar_bar_args = {'vertical': True},
            cmap='coolwarm',
            clim=self.clims[self.vis_mode[self.current_mode]]  # Ajustar el rango de colores

        )
        self.update_mesh(0)
        
        print("Done.")


    def update_mesh(self, frame):
        """Actualiza la visualización con el frame especificado"""
        self.current_frame = frame % len(self.thresholded_meshes)
        self.current_mesh = self.thresholded_meshes[self.current_frame]
        
        # Actualizar el mesh de manera eficiente
        #self.plotter.remove_actor(self.mesh_name)
        
        self.actor = self.plotter.add_mesh(
            self.current_mesh,
            scalars=self.vis_mode[self.current_mode],
            name=self.mesh_name,
            show_scalar_bar=True,
            scalar_bar_args = {'vertical': True},
            opacity=self.opacity / 100,  # Convertir a rango [0, 1]
            cmap='coolwarm',
            clim=self.clims[self.vis_mode[self.current_mode]]  # Ajustar el rango de colores

        )
        
        # Actualizar slider
        self.slider.GetSliderRepresentation().SetValue(self.current_frame)
        self.plotter.render()
    
    # Callbacks ...
    def slider_callback(self, value):
        """Callback para el slider"""
        frame = int(value)
        #self.slider.index = int(round(value))
        if frame != self.current_frame:
            self.update_mesh(frame)
    
    def opacity_callback(self, value):
        """Callback para el slider"""
        self.opacity = value
        self.update_mesh(self.current_frame)
    
    def animation_callback(self):
        """Callback para la animación"""
        if self.animation_running:
            self.update_mesh(self.current_frame + 1)
            QTimer.singleShot(100, self.animation_callback)
    
    def toggle_animation(self):
        """Inicia/pausa la animación"""
        self.animation_running = not self.animation_running
        if self.animation_running:
            self.animation_callback()
    
    def Right(self):
        """Avanza al siguiente frame"""
        self.update_mesh(self.current_frame + 1)
    def Left(self):
        """Retrocede al frame anterior"""
        self.update_mesh(self.current_frame - 1)
    def Change_VisMode(self):   
        """Cambia al siguiente modo de visualización"""
        self.current_mode = (self.current_mode + 1) % len(self.vis_mode)
        self.plotter.remove_actor('actor_vmode_txt')
        self.actor_vmode_txt = self.plotter.add_text(
            f"Mode: {self.vis_mode[self.current_mode]}",
            position='upper_right',
            font_size=12,
            name='actor_vmode_txt'
        )
        self.update_mesh(0)
        
    # Función para mostrar/ocultar el nodo inicial
    def inital_node(self):
        id_initial = self.inital_node_id #self.config['INITIAL_NODE_ID'] 
        if self.showing_initial_node:
            self.plotter.remove_actor('initial_node')
        else:
            if id_initial < len(self.current_mesh.points):
                initial_point = self.current_mesh.points[id_initial]
                sphere = pv.Sphere(radius=1.5, center=initial_point)
                self.initial_node_actor = self.plotter.add_mesh(
                    sphere,
                    color='black',
                    name='initial_node',
                    show_scalar_bar=False
                )
        self.showing_initial_node = not self.showing_initial_node
        self.update_mesh(self.current_frame)  # Actualizar el mesh para reflejar el cambio
        
        

def main(path, config):
    app = QApplication.instance() or QApplication([])
    
    # Cambia esta ruta por tu directorio real
    visualizer = DiffusionVisualizer(path, config)
    
    app.exec_()


if __name__ == '__main__':
    import sys
    if len(sys.argv) < 2:
        print("Usage: python arr3D_viewer.py <output_directory>")
        print("       Please, provide an output directory containing the configuration (.json) file.")
        sys.exit(1)
    else:
        output_dir, config_file = check_directory(sys.argv[1])
        print("Simulation finished", flush=True)
        main(output_dir, config_file)


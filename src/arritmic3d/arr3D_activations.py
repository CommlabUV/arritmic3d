import os
import json
import numpy as np
import arritmic3d

def load_json_list(file_path):
    """
    Load a JSON file containing a single list at top level.
    Raises ValueError if file does not exist or is not a valid JSON list.
    """
    if not os.path.isfile(file_path):
        raise ValueError(f"File not found: {file_path}")
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        raise ValueError(f"Invalid JSON in file {file_path}: {e}")
    if not isinstance(data, list):
        raise ValueError(f"File {file_path} must contain a JSON list at top level, got {type(data).__name__}.")
    return data

def resolve_activation_region(act_region, grid):
    """
    Resolve ACTIVATION_REGION to a list of node IDs.
    - act_region: int (group ID) | list | dict with "file" key
    - grid: pyvista grid with optional 'activation_region' field for group lookup
    Returns: list of node IDs
    Raises ValueError on invalid format or missing field.
    """
    if isinstance(act_region, list):
        return act_region
    elif isinstance(act_region, int):
        if 'activation_region' not in grid.point_data:
            raise ValueError("Grid missing 'activation_region' field for group lookup.")
        act_region_field = np.array(grid.point_data['activation_region'])
        nodes = np.where(act_region_field == act_region)[0].tolist()
        if not nodes:
            raise ValueError(f"No nodes found with activation_region group ID {act_region}.")
        return nodes
    elif isinstance(act_region, dict) and "file" in act_region:
        nodes = load_json_list(act_region["file"])
        return [int(n) for n in nodes]
    else:
        raise ValueError("ACTIVATION_REGION must be int, list, or dict with 'file' key.")

def resolve_first_activation_times(first_act_time, num_nodes):
    """
    Resolve FIRST_ACTIVATION_TIME to a list of times (one per node).
    - first_act_time: scalar | list | dict with "file" key
    - num_nodes: expected number of nodes
    Returns: list of floats (length == num_nodes)
    Raises ValueError if lengths don't match or file not found.
    """
    if isinstance(first_act_time, (int, float)):
        return [float(first_act_time)] * num_nodes
    elif isinstance(first_act_time, list):
        if len(first_act_time) != num_nodes:
            raise ValueError(f"FIRST_ACTIVATION_TIME list has {len(first_act_time)} elements, expected {num_nodes} (one per node).")
        return [float(t) for t in first_act_time]
    elif isinstance(first_act_time, dict) and "file" in first_act_time:
        times = load_json_list(first_act_time["file"])
        if len(times) != num_nodes:
            raise ValueError(f"File {first_act_time['file']} has {len(times)} times, expected {num_nodes} (one per node).")
        return [float(t) for t in times]
    else:
        raise ValueError("FIRST_ACTIVATION_TIME must be scalar, list, or dict with 'file' key.")

def parse_protocol_entry(protocol, grid):
    """
    Parse a PROTOCOL entry and resolve its components.
    Args:
        protocol: dict with keys 'ACTIVATION_REGION', 'BCL', 'N_STIMS_PACING', optional 'FIRST_ACTIVATION_TIME'
        grid: pyvista grid for resolving stimulation sites
    Returns:
        tuple: (node_first_time_pairs, bcl_list, n_stim_list)

    Parse a single PROTOCOL entry and return:
    - node_first_time_pairs: list of (node_id, first_activation_time) tuples
    - bcl_list: list of BCL values
    - n_stim_list: list of N_STIMS_PACING values

    Each (node_id, first_activation_time) pair indicates the node and the time for its first beat.
    The rest of the beats for each node are scheduled by accumulating BCLs and N_STIMS_PACING in schedule_activation.
    """
    # Resolve ACTIVATION_REGION
    nodes = resolve_activation_region(protocol['ACTIVATION_REGION'], grid)

    # Resolve FIRST_ACTIVATION_TIME
    if 'FIRST_ACTIVATION_TIME' in protocol:
        first_times = resolve_first_activation_times(protocol['FIRST_ACTIVATION_TIME'], len(nodes))
    else:
        # Default: use first BCL for all nodes (will be computed after BCL resolution)
        first_times = None

    # Resolve BCL and N_STIMS_PACING
    bcl_sn = protocol.get('BCL', [])
    n_stims_sn = protocol.get('N_STIMS_PACING', [])

    if not bcl_sn or not n_stims_sn:
        raise ValueError("PROTOCOL entry must include 'BCL' and 'N_STIMS_PACING'.")

    # Normalize to lists
    if not isinstance(bcl_sn, list):
        bcl_sn = [bcl_sn]
    if not isinstance(n_stims_sn, list):
        n_stims_sn = [n_stims_sn]

    # Pad shorter list with last value
    if len(n_stims_sn) < len(bcl_sn):
        n_stims_sn = n_stims_sn + [n_stims_sn[-1]] * (len(bcl_sn) - len(n_stims_sn))
    elif len(bcl_sn) < len(n_stims_sn):
        bcl_sn = bcl_sn + [bcl_sn[-1]] * (len(n_stims_sn) - len(bcl_sn))

    # If no explicit first_activation_time, default to first BCL
    if first_times is None:
        first_times = [float(bcl_sn[0])] * len(nodes)

    node_first_time_pairs = list(zip(nodes, first_times))

    return node_first_time_pairs, bcl_sn, n_stims_sn

def load_activate_nodes_from_file(file_path):
    """
    Load ACTIVATE_NODES from external JSON file.
    Expected format: [[node_id, activation_time, beat], ...]
    Returns: list of (node_id, activation_time, beat) tuples
    Raises ValueError if format is invalid.
    """
    data = load_json_list(file_path)
    result = []
    for i, entry in enumerate(data):
        if not isinstance(entry, list) or len(entry) != 3:
            raise ValueError(f"Entry {i} in {file_path} must be [node_id, activation_time, beat], got {entry}.")
        try:
            node_id = int(entry[0])
            time = float(entry[1])
            beat = int(entry[2])
        except (ValueError, TypeError) as e:
            raise ValueError(f"Entry {i} in {file_path}: invalid type conversion: {e}")
        result.append((node_id, time, beat))
    return result

def add_activate_nodes_entries(activations, entries):
    """
    Add a list of (node_id, time, beat) tuples to the activations dict.
    """
    for node_id, time, beat_num in entries:
        if time not in activations:
            activations[time] = [[node_id], beat_num]
        else:
            activations[time][0].append(node_id)
            activations[time][1] = beat_num

def schedule_activation(cfg, grid, tissue):
    """
    Schedule activation events based on the configuration.

    Args:
        cfg: dict with activation configuration (may include PROTOCOL and/or ACTIVATE_NODES)
        grid: pyvista grid representing the tissue
        tissue: tissue object with SetSystemEvent method

    Returns:
        activations: dict mapping activation_time (float) -> [list of node_ids, beat_number]
    """
    activations = {}

    if "PROTOCOL" in cfg:
        protocols = cfg['PROTOCOL']
        for protocol in protocols:
            # Use helper to parse protocol
            node_time_pairs, bcl_sn, n_stims_sn = parse_protocol_entry(protocol, grid)

            # Print pacing protocol summary
            print("Pacing protocol:", flush=True)
            for i, (n_stim, bcl) in enumerate(zip(n_stims_sn, bcl_sn), 1):
                print(f"  S{i}: N_STIMS={n_stim}, BCL={bcl}", flush=True)
            # Build list of (node_id, activation_time, beat) tuples
            entries = []
            for node, first_time in node_time_pairs:
                # All the nodes have the same beating schedule during a protocol entry
                # Initialize beat counting and increase beat after each activation
                beat = protocol.get('FIRST_BEAT_NUM', 1)
                t = first_time
                # iterate over blocks with index to access next block BCL
                for bi, (n_stim, bcl) in enumerate(zip(n_stims_sn,bcl_sn)):
                    for si in range(n_stim):
                        entries.append((node, t, beat))
                        beat += 1
                        # step uses current bcl unles we are in the last stim
                        if bi + 1 < len(bcl_sn) and (si + 1) >= n_stim:
                            next_cl = bcl_sn[bi + 1]
                        else:
                            next_cl = bcl
                        t += next_cl
            add_activate_nodes_entries(activations, entries)

    if "ACTIVATE_NODES" in cfg:
        for activation in cfg['ACTIVATE_NODES']:
            # Check if it's a file reference
            if isinstance(activation, dict) and "file" in activation:
                entries = load_activate_nodes_from_file(activation["file"])
                add_activate_nodes_entries(activations, entries)
            else:
                initial_nodes = resolve_activation_region(activation['ACTIVATION_REGION'], grid)
                entries = []
                for time, beat_num in activation['ACTIVATION_TIMES']:
                    for node_id in initial_nodes:
                        entries.append((node_id, time, beat_num))
                add_activate_nodes_entries(activations, entries)

    # Insert all activation times as system events
    for activation_time in activations:
        tissue.SetSystemEvent(arritmic3d.SystemEventType.EXT_ACTIVATION, activation_time)

    return activations

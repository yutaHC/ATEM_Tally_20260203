const { InstanceBase, runEntrypoint, InstanceStatus } = require('@companion-module/base')
const dgram = require('dgram')

class TallyInstance extends InstanceBase {
	async init(config) {
		this.config = config
		this.updateStatus(InstanceStatus.Ok)
		this.updateActions()
	}

	async destroy() {
		this.log('debug', 'destroy')
	}

	async configUpdated(config) {
		this.config = config
		this.updateActions()
	}

	getConfigFields() {
		return [
			{
				type: 'textinput',
				id: 'host',
				label: 'Target IP (M5Atom IP Address)',
				width: 8,
				regex: this.REGEX_IP
			},
			{
				type: 'number',
				id: 'port',
				label: 'Target Port',
				width: 4,
				default: 8888,
				min: 1,
				max: 65535,
			}
		]
	}

	updateActions() {
		this.setActionDefinitions({
			pgm: {
				name: 'Set Program (Red)',
				options: [],
				callback: async () => {
					this.sendUdpCommand('pgm')
				},
			},
			pvw: {
				name: 'Set Preview (Green)',
				options: [],
				callback: async () => {
					this.sendUdpCommand('pvw')
				},
			},
			off: {
				name: 'Set Off',
				options: [],
				callback: async () => {
					this.sendUdpCommand('off')
				},
			},
			identify: {
				name: 'Identify (Blink Blue)',
				options: [],
				callback: async () => {
					this.sendUdpCommand('identify')
				},
			},
			dim: {
				name: 'Set Brightness',
				options: [
					{
						type: 'number',
						label: 'Brightness (0-255)',
						id: 'brightness',
						min: 0,
						max: 255,
						default: 128,
						required: true,
					}
				],
				callback: async (action) => {
					this.sendUdpCommand(`dim:${action.options.brightness}`)
				},
			}
		})
	}

	sendUdpCommand(cmd) {
		if (this.config.host) {
			const client = dgram.createSocket('udp4')
			client.send(cmd, this.config.port || 8888, this.config.host, (err) => {
				client.close()
			})
		}
	}
}

runEntrypoint(TallyInstance, [])

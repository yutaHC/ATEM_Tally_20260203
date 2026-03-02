const { InstanceBase, runEntrypoint, InstanceStatus } = require('@companion-module/base')
const dgram = require('dgram')
const mdns = require('multicast-dns')()

class TallyInstance extends InstanceBase {
	constructor(internal) {
		super(internal)
		this.discoveredDevices = {} // { hostname: { ip, port, name } }
	}

	async init(config) {
		this.config = config
		this.updateStatus(InstanceStatus.Ok)
		this.startMdnsDiscovery()
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
				type: 'dropdown',
				id: 'device',
				label: 'Tally Device (discovered via mDNS)',
				width: 12,
				default: '',
				choices: this.getDeviceChoices(),
				allowCustom: true,
			},
			{
				type: 'number',
				id: 'port',
				label: 'UDP Port (override)',
				width: 4,
				default: 8888,
				min: 1,
				max: 65535,
			}
		]
	}

	getDeviceChoices() {
		const choices = Object.entries(this.discoveredDevices).map(([key, d]) => ({
			id: d.ip,
			label: `${d.name} (${d.ip}:${d.port})`,
		}))
		if (choices.length === 0) {
			choices.push({ id: '', label: '--- Searching for devices... ---' })
		}
		return choices
	}

	startMdnsDiscovery() {
		// _tally._udp サービスを定期的に検索する
		const query = () => {
			mdns.query({ questions: [{ name: '_tally._udp.local', type: 'PTR' }] })
		}

		// SRV/TXT/Aレコードから情報を抽出する
		mdns.on('response', (response) => {
			const srvRecord = response.answers.find((a) => a.type === 'SRV')
			const aRecord = response.answers.find((a) => a.type === 'A')
			const txtRecord = response.answers.find((a) => a.type === 'TXT')

			if (srvRecord && aRecord) {
				const ip = aRecord.data
				const port = srvRecord.data.port
				let deviceDisplayName = srvRecord.data.target.replace('.local', '')

				if (txtRecord && txtRecord.data) {
					const nameEntry = txtRecord.data.find((b) =>
						Buffer.isBuffer(b) && b.toString().startsWith('name=')
					)
					if (nameEntry) {
						deviceDisplayName = nameEntry.toString().replace('name=', '')
					}
				}

				const key = ip
				if (!this.discoveredDevices[key] || this.discoveredDevices[key].name !== deviceDisplayName) {
					this.discoveredDevices[key] = { ip, port, name: deviceDisplayName }
					this.log('debug', `Discovered: ${deviceDisplayName} at ${ip}:${port}`)
					// 設定フィールドのドロップダウンを更新する
					this.setVariableDefinitions([])
				}
			}
		})

		// 起動時と30秒毎に検索
		query()
		this._mdnsInterval = setInterval(query, 30000)
	}

	updateActions() {
		this.setActionDefinitions({
			pgm: {
				name: 'Set Program (Red)',
				options: [],
				callback: async () => this.sendUdpCommand('pgm'),
			},
			pvw: {
				name: 'Set Preview (Green)',
				options: [],
				callback: async () => this.sendUdpCommand('pvw'),
			},
			off: {
				name: 'Set Off',
				options: [],
				callback: async () => this.sendUdpCommand('off'),
			},
			identify: {
				name: 'Identify (Blink Blue)',
				options: [],
				callback: async () => this.sendUdpCommand('identify'),
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
					},
				],
				callback: async (action) => this.sendUdpCommand(`dim:${action.options.brightness}`),
			},
		})
	}

	sendUdpCommand(cmd) {
		const ip = this.config.device
		const port = this.config.port || 8888
		if (ip) {
			const client = dgram.createSocket('udp4')
			client.send(cmd, port, ip, (err) => {
				if (err) this.log('error', `UDP send error: ${err.message}`)
				client.close()
			})
		} else {
			this.log('warn', 'No device selected. Configure the module first.')
		}
	}
}

runEntrypoint(TallyInstance, [])
